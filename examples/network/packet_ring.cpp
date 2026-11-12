// TPACKET_V3 block ownership, not the Ethernet chip's hardware DMA ring.
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
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: packet_ring INTERFACE COUNT [TIMEOUT_SECONDS]\n";
        return 2;
    }
    int socket_fd = -1;
    void* mapping = MAP_FAILED;
    constexpr std::size_t block_size = 1u << 20;
    constexpr unsigned block_count = 2;
    constexpr std::size_t mapping_size = block_size * block_count;
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
        socket_fd = socket(AF_PACKET, SOCK_RAW | SOCK_CLOEXEC, 0);
        if (socket_fd < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        const int version = TPACKET_V3;
        const int ignore_outgoing = 1;
        tpacket_req3 request{};
        request.tp_block_size = block_size;
        request.tp_block_nr = block_count;
        request.tp_frame_size = 2048;
        request.tp_frame_nr = (block_size / request.tp_frame_size) * block_count;
        request.tp_retire_blk_tov = 50;
        if (setsockopt(socket_fd, SOL_PACKET, PACKET_VERSION, &version,
                       sizeof(version)) < 0 ||
            setsockopt(socket_fd, SOL_PACKET, PACKET_IGNORE_OUTGOING, &ignore_outgoing,
                       sizeof(ignore_outgoing)) < 0 ||
            setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, &request,
                       sizeof(request)) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        mapping = mmap(nullptr, mapping_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       socket_fd, 0);
        if (mapping == MAP_FAILED) {
            throw std::runtime_error(std::strerror(errno));
        }
        sockaddr_ll local{};
        local.sll_family = AF_PACKET;
        local.sll_protocol = htons(ETH_P_ALL);
        local.sll_ifindex = static_cast<int>(interface_index);
        if (bind(socket_fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
        unsigned block_index = 0;
        unsigned matched = 0;
        unsigned packets = 0;
        unsigned blocks = 0;
        std::cout << "{\"event\":\"ready\",\"mode\":\"TPACKET_V3\"}" << std::endl;
        while (matched < static_cast<unsigned>(expected_count)) {
            auto* block_bytes =
                static_cast<std::uint8_t*>(mapping) + block_index * block_size;
            auto* block = reinterpret_cast<tpacket_block_desc*>(block_bytes);
            if ((__atomic_load_n(&block->hdr.bh1.block_status, __ATOMIC_ACQUIRE) &
                 TP_STATUS_USER) == 0) {
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
                    throw std::runtime_error("ring poll failed");
                }
                if (result == 0) {
                    break;
                }
                continue;
            }
            // mmap is page-aligned and each block is a page multiple; these
            // kernel UAPI metadata structs have their documented alignment.
            ++blocks;
            const auto block_length = block->hdr.bh1.blk_len;
            auto packet_offset =
                static_cast<std::size_t>(block->hdr.bh1.offset_to_first_pkt);
            const auto packet_count = block->hdr.bh1.num_pkts;
            bool invalid =
                block->version != TPACKET_V3 || block_length > block_size ||
                block_length < sizeof(tpacket_block_desc) ||
                packet_count > block_size / sizeof(tpacket3_hdr) ||
                (packet_count != 0 && packet_offset < sizeof(tpacket_block_desc));
            for (unsigned packet_index = 0; !invalid && packet_index < packet_count;
                 ++packet_index) {
                if (packet_offset % alignof(tpacket3_hdr) != 0 ||
                    packet_offset > block_length ||
                    sizeof(tpacket3_hdr) > block_length - packet_offset) {
                    invalid = true;
                    break;
                }
                const auto* header =
                    reinterpret_cast<const tpacket3_hdr*>(block_bytes + packet_offset);
                const auto available = block_length - packet_offset;
                if (header->tp_mac < sizeof(tpacket3_hdr) ||
                    header->tp_mac > available ||
                    header->tp_snaplen > available - header->tp_mac ||
                    header->tp_snaplen > header->tp_len) {
                    invalid = true;
                    break;
                }
                const auto bytes = std::span<const std::uint8_t>(
                    block_bytes + packet_offset + header->tp_mac, header->tp_snaplen);
                ++packets;
                const bool stripped_vlan =
                    (header->tp_status & TP_STATUS_VLAN_VALID) != 0;
                auto decoded = hfs::net::decode_frame(bytes);
                if (stripped_vlan) {
                    decoded.kind = hfs::net::Kind::unsupported;
                    std::cout
                        << "{\"event\":\"unsupported\",\"reason\":\"VLAN metadata\"}\n";
                }
                if (decoded.kind == hfs::net::Kind::arp ||
                    (decoded.kind == hfs::net::Kind::udp &&
                     decoded.destination_port == hfs::net::lab_port)) {
                    std::cout << hfs::net::describe_frame(bytes) << std::endl;
                }
                if (decoded.kind == hfs::net::Kind::udp &&
                    decoded.destination_port == hfs::net::lab_port &&
                    decoded.sequence) {
                    ++matched;
                }
                if (packet_index + 1 < packet_count) {
                    if (header->tp_next_offset < sizeof(tpacket3_hdr) ||
                        header->tp_next_offset > available ||
                        header->tp_next_offset < header->tp_mac + header->tp_snaplen) {
                        invalid = true;
                        break;
                    }
                    packet_offset += header->tp_next_offset;
                }
            }
            // Return the block even when its metadata failed a check. No pointer
            // into it survives this release or the subsequent unmap.
            __atomic_store_n(&block->hdr.bh1.block_status, TP_STATUS_KERNEL,
                             __ATOMIC_RELEASE);
            block_index = (block_index + 1) % block_count;
            if (invalid) {
                throw std::runtime_error("invalid TPACKET_V3 block metadata");
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                break;
            }
        }
        tpacket_stats_v3 statistics{};
        socklen_t statistics_size = sizeof(statistics);
        if (getsockopt(socket_fd, SOL_PACKET, PACKET_STATISTICS, &statistics,
                       &statistics_size) < 0) {
            throw std::runtime_error("PACKET_STATISTICS failed");
        }
        munmap(mapping, mapping_size);
        mapping = MAP_FAILED;
        close(socket_fd);
        socket_fd = -1;
        std::cout << "{\"event\":\"summary\",\"matched\":" << matched
                  << ",\"packets\":" << packets << ",\"blocks\":" << blocks
                  << ",\"kernel_drops\":" << statistics.tp_drops
                  << ",\"queue_freezes\":" << statistics.tp_freeze_q_cnt << "}\n";
        return matched >= static_cast<unsigned>(expected_count) ? 0 : 1;
    } catch (const std::exception& error) {
        if (mapping != MAP_FAILED) {
            munmap(mapping, mapping_size);
        }
        if (socket_fd >= 0) {
            close(socket_fd);
        }
        std::cerr << error.what() << '\n';
        return 2;
    }
}
