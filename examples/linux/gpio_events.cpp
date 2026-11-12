#include <cerrno>
#include <charconv>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <linux/gpio.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

bool offset(const char* text, unsigned& value) {
    std::string offset_text = text;
    auto [parsed_end, parse_error] = std::from_chars(
        offset_text.data(), offset_text.data() + offset_text.size(), value);
    return parse_error == std::errc{} &&
           parsed_end == offset_text.data() + offset_text.size();
}

int main(int argc, char** argv) {
    unsigned led_offset = 0, button_offset = 0;
    if (argc != 4 || !offset(argv[2], led_offset) || !offset(argv[3], button_offset) ||
        led_offset == button_offset) {
        std::cerr << "usage: gpio_events CHIP LED_OFFSET BUTTON_OFFSET\n";
        return 2;
    }
    int chip_fd = open(argv[1], O_RDONLY | O_CLOEXEC);
    if (chip_fd < 0) {
        std::cerr << std::strerror(errno) << '\n';
        return 1;
    }
    gpio_v2_line_request led_request{};
    led_request.offsets[0] = led_offset;
    led_request.num_lines = 1;
    std::strcpy(led_request.consumer, "hfs-led");
    led_request.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &led_request) < 0) {
        std::cerr << "request LED: " << std::strerror(errno) << '\n';
        close(chip_fd);
        return 1;
    }
    gpio_v2_line_request button_request{};
    button_request.offsets[0] = button_offset;
    button_request.num_lines = 1;
    std::strcpy(button_request.consumer, "hfs-button");
    button_request.config.flags = GPIO_V2_LINE_FLAG_INPUT |
                                  GPIO_V2_LINE_FLAG_EDGE_FALLING |
                                  GPIO_V2_LINE_FLAG_BIAS_DISABLED;
    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &button_request) < 0) {
        std::cerr << "request button: " << std::strerror(errno) << '\n';
        close(led_request.fd);
        close(chip_fd);
        return 1;
    }
    uint64_t last_edge_ns = 0;
    bool led_is_on = false;
    unsigned press_count = 0;
    int result = 0;
    while (press_count < 5) {
        pollfd poll_descriptor{button_request.fd, POLLIN, 0};
        int poll_result = poll(&poll_descriptor, 1, 30000);
        if (poll_result < 0 && errno == EINTR) {
            continue;
        }
        if (poll_result <= 0 ||
            (poll_descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            std::cerr << "GPIO timeout/error\n";
            result = 1;
            break;
        }
        if (!(poll_descriptor.revents & POLLIN)) {
            continue;
        }
        gpio_v2_line_event event{};
        auto bytes_read = read(button_request.fd, &event, sizeof(event));
        if (bytes_read < 0 && errno == EINTR) {
            continue;
        }
        if (bytes_read != sizeof(event)) {
            result = 1;
            break;
        }
        if (last_edge_ns && event.timestamp_ns - last_edge_ns < 20000000) {
            continue;
        }
        last_edge_ns = event.timestamp_ns;
        led_is_on = !led_is_on;
        gpio_v2_line_values values{};
        values.mask = 1;
        values.bits = led_is_on ? 1 : 0;
        if (ioctl(led_request.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &values) < 0) {
            result = 1;
            break;
        }
        std::cout << "accepted edge " << ++press_count << " led=" << led_is_on << '\n';
    }
    // This simple edge lockout is a baseline, not a stable-level debounce proof.
    gpio_v2_line_values led_off{};
    led_off.mask = 1;
    ioctl(led_request.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &led_off);
    close(button_request.fd);
    close(led_request.fd);
    close(chip_fd);
    return result;
}
