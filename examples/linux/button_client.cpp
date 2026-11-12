#include <cerrno>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: button_client /dev/mybutton EVENT_COUNT\n";
        return 2;
    }
    unsigned event_count = 0;
    const std::string count_argument = argv[2];
    auto [parsed_end, parse_error] =
        std::from_chars(count_argument.data(),
                        count_argument.data() + count_argument.size(), event_count);
    if (parse_error != std::errc{} ||
        parsed_end != count_argument.data() + count_argument.size() ||
        event_count == 0 || event_count > 1000) {
        return 2;
    }
    int button_fd = open(argv[1], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (button_fd < 0) {
        std::cerr << std::strerror(errno) << '\n';
        return 1;
    }
    unsigned events_received = 0;
    while (events_received < event_count) {
        pollfd button_poll{button_fd, POLLIN, 0};
        int poll_result = poll(&button_poll, 1, 30000);
        if (poll_result < 0 && errno == EINTR) {
            continue;
        }
        if (poll_result <= 0 ||
            (button_poll.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            std::cerr << "timeout, error, or device offline\n";
            close(button_fd);
            return 1;
        }
        if (!(button_poll.revents & POLLIN)) {
            continue;
        }
        char buffer[64];
        auto bytes_read = read(button_fd, buffer, sizeof(buffer));
        if (bytes_read < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        if (bytes_read <= 0) {
            std::cerr << "read: " << std::strerror(errno) << '\n';
            close(button_fd);
            return 1;
        }
        std::cout.write(buffer, bytes_read);
        ++events_received;
    }
    close(button_fd);
}
