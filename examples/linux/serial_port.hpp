#pragma once
#include <cerrno>
#include <chrono>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>
#include "../common/protocol.hpp"

namespace hfs {
class Serial {
    int serial_fd_ = -1;
    using Clock = std::chrono::steady_clock;
    void ready(short events, Clock::time_point deadline) {
        while (true) {
            const auto remaining_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
                                                                      Clock::now())
                    .count();
            if (remaining_ms <= 0) {
                throw std::runtime_error("UART deadline expired");
            }
            pollfd poll_descriptor{serial_fd_, events, 0};
            const int poll_result =
                poll(&poll_descriptor, 1, static_cast<int>(remaining_ms));
            if (poll_result < 0 && errno == EINTR) {
                continue;
            }
            if (poll_result < 0) {
                throw std::runtime_error(std::strerror(errno));
            }
            if (poll_result == 0) {
                throw std::runtime_error("UART deadline expired");
            }
            if (poll_descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("UART disconnected");
            }
            if (poll_descriptor.revents & events) {
                return;
            }
        }
    }

  public:
    explicit Serial(const char* path) {
        serial_fd_ = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
        if (serial_fd_ < 0) {
            throw std::runtime_error(std::string("open UART: ") + std::strerror(errno));
        }
        termios settings{};
        if (tcgetattr(serial_fd_, &settings) < 0) {
            close(serial_fd_);
            serial_fd_ = -1;
            throw std::runtime_error("tcgetattr failed");
        }
        cfmakeraw(&settings);
        settings.c_iflag &= ~(IXON | IXOFF | IXANY);
        cfsetispeed(&settings, B115200);
        cfsetospeed(&settings, B115200);
        settings.c_cflag = (settings.c_cflag & ~(CSIZE | PARENB | CSTOPB | CRTSCTS)) |
                           CS8 | CLOCAL | CREAD;
        settings.c_cc[VMIN] = 0;
        settings.c_cc[VTIME] = 0;
        if (tcsetattr(serial_fd_, TCSANOW, &settings) < 0 ||
            tcflush(serial_fd_, TCIOFLUSH) < 0) {
            close(serial_fd_);
            serial_fd_ = -1;
            throw std::runtime_error("UART configuration failed");
        }
    }
    ~Serial() {
        if (serial_fd_ >= 0) {
            close(serial_fd_);
        }
    }
    Serial(const Serial&) = delete;
    Serial& operator=(const Serial&) = delete;
    std::string transact(unsigned id, const std::string& command) {
        const std::string request = std::to_string(id) + " " + command;
        Request parsed_request;
        if (request.size() > 63 || !parse(request.c_str(), parsed_request)) {
            throw std::runtime_error("invalid command");
        }
        const auto deadline = Clock::now() + std::chrono::seconds(2);
        const std::string bytes = request + "\n";
        std::size_t bytes_sent = 0;
        while (bytes_sent < bytes.size()) {
            ready(POLLOUT, deadline);
            const auto transferred_bytes =
                write(serial_fd_, bytes.data() + bytes_sent, bytes.size() - bytes_sent);
            if (transferred_bytes < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            if (transferred_bytes <= 0) {
                throw std::runtime_error("UART write failed");
            }
            bytes_sent += static_cast<std::size_t>(transferred_bytes);
        }
        Line line;
        while (true) {
            ready(POLLIN, deadline);
            char byte;
            const auto transferred_bytes = read(serial_fd_, &byte, 1);
            if (transferred_bytes < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            if (transferred_bytes <= 0) {
                throw std::runtime_error("UART read failed");
            }
            const auto result = line.feed(byte);
            if (result == Line::Result::overflow) {
                throw std::runtime_error("overlong reply");
            }
            if (result == Line::Result::ready) {
                const std::string reply = line.text();
                const std::string prefix = std::to_string(id) + " ";
                if (reply.compare(0, prefix.size(), prefix) != 0) {
                    throw std::runtime_error("unexpected reply ID");
                }
                if (reply.compare(prefix.size(), 3, "OK ") != 0) {
                    throw std::runtime_error("device rejected command: " + reply);
                }
                const auto value_offset = prefix.size() + 3;
                long long value = 0;
                const auto [parsed_end, error] = std::from_chars(
                    reply.data() + value_offset, reply.data() + reply.size(), value);
                if (error != std::errc{} || parsed_end != reply.data() + reply.size()) {
                    throw std::runtime_error("malformed numeric reply");
                }
                if (parsed_request.op == Op::ping && value != 1) {
                    throw std::runtime_error("unexpected PING result");
                }
                if (parsed_request.op == Op::led && value != parsed_request.led) {
                    throw std::runtime_error("LED acknowledgement mismatch");
                }
                if (parsed_request.op == Op::count &&
                    (value < 0 || value > 4294967295LL)) {
                    throw std::runtime_error("COUNT outside uint32 range");
                }
                if (parsed_request.op == Op::temp &&
                    (value < INT32_MIN || value > INT32_MAX)) {
                    throw std::runtime_error("TEMP outside int32 range");
                }
                return reply;
            }
        }
    }
};
} // namespace hfs
