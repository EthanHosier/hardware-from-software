#include "serial_port.hpp"
#include <charconv>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: capstone /dev/mybutton /dev/serial0 EVENT_COUNT\n";
        return 2;
    }
    unsigned event_count = 0;
    std::string count_argument = argv[3];
    auto [parsed_end, parse_error] =
        std::from_chars(count_argument.data(),
                        count_argument.data() + count_argument.size(), event_count);
    if (parse_error != std::errc{} ||
        parsed_end != count_argument.data() + count_argument.size() || !event_count ||
        event_count > 1000) {
        return 2;
    }
    const int button_fd = open(argv[1], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (button_fd < 0) {
        std::cerr << std::strerror(errno) << '\n';
        return 1;
    }
    try {
        hfs::Serial serial(argv[2]);
        for (unsigned request_id = 1; request_id <= event_count;) {
            pollfd button_poll{button_fd, POLLIN, 0};
            int poll_result = poll(&button_poll, 1, 30000);
            if (poll_result < 0 && errno == EINTR) {
                continue;
            }
            if (poll_result <= 0 ||
                (button_poll.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                throw std::runtime_error("button timeout/offline");
            }
            if (!(button_poll.revents & POLLIN)) {
                continue;
            }
            char bytes[64];
            const auto bytes_read = read(button_fd, bytes, sizeof(bytes));
            if (bytes_read < 0 && (errno == EAGAIN || errno == EINTR)) {
                continue;
            }
            if (bytes_read <= 0) {
                throw std::runtime_error("button read failed");
            }
            const std::string record(bytes, static_cast<std::size_t>(bytes_read));
            const auto separator_index = record.find(' ');
            if (separator_index == std::string::npos ||
                record.size() != separator_index + 3 || record.back() != '\n' ||
                (record[separator_index + 1] != '0' &&
                 record[separator_index + 1] != '1')) {
                throw std::runtime_error("bad button record");
            }
            const std::string command =
                std::string("LED ") + record[separator_index + 1];
            const auto reply = serial.transact(request_id, command);
            if (reply !=
                std::to_string(request_id) + " OK " + record[separator_index + 1]) {
                throw std::runtime_error("LED acknowledgement mismatch");
            }
            std::cout << "button " << record.substr(0, separator_index) << " -> "
                      << reply << '\n';
            ++request_id;
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        close(button_fd);
        return 1;
    }
    close(button_fd);
}
