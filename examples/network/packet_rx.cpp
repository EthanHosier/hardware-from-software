// AF_PACKET receives Ethernet bytes while Linux still owns the network device.
#include "wire.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: packet_rx INTERFACE COUNT [TIMEOUT_SECONDS]\n";
        return 2;
    }
    int socket_fd = -1;
    try {
        auto number = [](const char* text, int maximum) {
            std::size_t consumed = 0;
            const auto value = std::stoi(text, &consumed);
            if (consumed != std::strlen(text) || value < 1 || value > maximum) {
                throw std::runtime_error("invalid numeric argument");
            }
            return value;
        };
        const auto expected_count = number(argv[2], 10000);
        const auto timeout = number(argc == 4 ? argv[3] : "30", 300);
        const auto interface_index = if_nametoindex(argv[1]);
        if (interface_index == 0) {
            throw std::runtime_error("unknown interface");
        }
        socket_fd = socket(AF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(ETH_P_ALL));
        if (socket_fd < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        sockaddr_ll local{};
        local.sll_family = AF_PACKET;
        local.sll_protocol = htons(ETH_P_ALL);
        local.sll_ifindex = static_cast<int>(interface_index);
        const int ignore_outgoing = 1;
        if (setsockopt(socket_fd, SOL_PACKET, PACKET_IGNORE_OUTGOING, &ignore_outgoing,
                       sizeof(ignore_outgoing)) < 0 ||
            setsockopt(socket_fd, SOL_PACKET, PACKET_AUXDATA, &ignore_outgoing,
                       sizeof(ignore_outgoing)) < 0 ||
            bind(socket_fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
        unsigned matched = 0;
        unsigned truncated = 0;
        unsigned packets = 0;
        std::cout << "{\"event\":\"ready\",\"mode\":\"AF_PACKET recv\"}" << std::endl;
        while (matched < static_cast<unsigned>(expected_count)) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now())
                    .count();
            if (remaining <= 0) {
                break;
            }
            pollfd ready{socket_fd, POLLIN, 0};
            const auto result = poll(&ready, 1, static_cast<int>(remaining));
            if (result < 0 && errno == EINTR) {
                continue;
            }
            if (result < 0 || (ready.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                throw std::runtime_error("packet poll failed");
            }
            if (result == 0) {
                break;
            }
            std::array<std::uint8_t, 65536> storage{};
            alignas(cmsghdr) std::array<char, CMSG_SPACE(sizeof(tpacket_auxdata))>
                control{};
            iovec vector{storage.data(), storage.size()};
            msghdr message{};
            message.msg_iov = &vector;
            message.msg_iovlen = 1;
            message.msg_control = control.data();
            message.msg_controllen = control.size();
            const auto received =
                recvmsg(socket_fd, &message, MSG_TRUNC | MSG_DONTWAIT);
            if (received < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            if (received < 0) {
                throw std::runtime_error(std::strerror(errno));
            }
            ++packets;
            if (static_cast<std::size_t>(received) > storage.size()) {
                ++truncated;
                continue;
            }
            const auto bytes =
                std::span(storage).first(static_cast<std::size_t>(received));
            bool stripped_vlan = false;
            for (auto* ancillary = CMSG_FIRSTHDR(&message); ancillary != nullptr;
                 ancillary = CMSG_NXTHDR(&message, ancillary)) {
                if (ancillary->cmsg_level == SOL_PACKET &&
                    ancillary->cmsg_type == PACKET_AUXDATA &&
                    ancillary->cmsg_len >= CMSG_LEN(sizeof(tpacket_auxdata))) {
                    tpacket_auxdata metadata{};
                    std::memcpy(&metadata, CMSG_DATA(ancillary), sizeof(metadata));
                    stripped_vlan = (metadata.tp_status & TP_STATUS_VLAN_VALID) != 0;
                }
            }
            if (stripped_vlan || (message.msg_flags & MSG_CTRUNC)) {
                std::cout << "{\"event\":\"unsupported\",\"reason\":\"VLAN metadata or "
                             "truncated ancillary data\"}\n";
                continue;
            }
            const auto decoded = hfs::net::decode_frame(bytes);
            if (decoded.kind == hfs::net::Kind::arp ||
                (decoded.kind == hfs::net::Kind::udp &&
                 decoded.destination_port == hfs::net::lab_port)) {
                std::cout << hfs::net::describe_frame(bytes) << std::endl;
            }
            if (decoded.kind == hfs::net::Kind::udp &&
                decoded.destination_port == hfs::net::lab_port && decoded.sequence) {
                ++matched;
            }
        }
        tpacket_stats statistics{};
        socklen_t statistics_size = sizeof(statistics);
        if (getsockopt(socket_fd, SOL_PACKET, PACKET_STATISTICS, &statistics,
                       &statistics_size) < 0) {
            throw std::runtime_error("PACKET_STATISTICS failed");
        }
        close(socket_fd);
        socket_fd = -1;
        std::cout << "{\"event\":\"summary\",\"matched\":" << matched
                  << ",\"packets\":" << packets << ",\"truncated\":" << truncated
                  << ",\"kernel_drops\":" << statistics.tp_drops << "}\n";
        return matched == static_cast<unsigned>(expected_count) ? 0 : 1;
    } catch (const std::exception& error) {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
        std::cerr << error.what() << '\n';
        return 2;
    }
}
