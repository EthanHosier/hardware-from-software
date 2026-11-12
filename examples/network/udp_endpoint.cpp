// A finite IPv4 UDP responder. Run inside hfs-lab for the isolated exercises.
#include "wire.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "usage: udp_endpoint BIND_IPV4 PORT COUNT [TIMEOUT_SECONDS]\n";
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
        const auto port = number(argv[2], 65535);
        const auto expected_count = number(argv[3], 10000);
        const auto timeout = number(argc == 5 ? argv[4] : "30", 300);
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port = htons(static_cast<std::uint16_t>(port));
        if (inet_pton(AF_INET, argv[1], &local.sin_addr) != 1) {
            throw std::runtime_error("BIND_IPV4 must be an IPv4 literal");
        }
        socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
        if (socket_fd < 0 ||
            bind(socket_fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        std::cout << "{\"event\":\"ready\",\"port\":" << port << "}" << std::endl;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
        unsigned accepted = 0;
        unsigned rejected = 0;
        while (accepted < static_cast<unsigned>(expected_count)) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now())
                    .count();
            if (remaining <= 0) {
                break;
            }
            pollfd ready{socket_fd, POLLIN, 0};
            const auto poll_result = poll(&ready, 1, static_cast<int>(remaining));
            if (poll_result < 0 && errno == EINTR) {
                continue;
            }
            if (poll_result < 0 || (ready.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                throw std::runtime_error("UDP poll failed");
            }
            if (poll_result == 0) {
                break;
            }
            std::array<std::uint8_t, 2048> bytes{};
            sockaddr_in peer{};
            socklen_t peer_size = sizeof(peer);
            const auto received = recvfrom(
                socket_fd, bytes.data(), bytes.size(), MSG_TRUNC | MSG_DONTWAIT,
                reinterpret_cast<sockaddr*>(&peer), &peer_size);
            if (received < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            if (received < 0) {
                throw std::runtime_error(std::strerror(errno));
            }
            if (static_cast<std::size_t>(received) > bytes.size()) {
                ++rejected;
                continue;
            }
            const auto message =
                std::span(bytes).first(static_cast<std::size_t>(received));
            const auto sequence = hfs::net::decode_message(message);
            if (!sequence) {
                ++rejected;
                std::cout << "{\"event\":\"rejected\",\"length\":" << received << "}\n";
                continue;
            }
            const auto sent =
                sendto(socket_fd, message.data(), message.size(), MSG_DONTWAIT,
                       reinterpret_cast<sockaddr*>(&peer), peer_size);
            if (sent != received) {
                throw std::runtime_error("UDP reply failed");
            }
            ++accepted;
            std::cout << "{\"event\":\"reply\",\"sequence\":" << *sequence << "}"
                      << std::endl;
        }
        close(socket_fd);
        socket_fd = -1;
        std::cout << "{\"event\":\"summary\",\"accepted\":" << accepted
                  << ",\"rejected\":" << rejected << "}\n";
        return accepted == static_cast<unsigned>(expected_count) ? 0 : 1;
    } catch (const std::exception& error) {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
        std::cerr << error.what() << '\n';
        return 2;
    }
}
