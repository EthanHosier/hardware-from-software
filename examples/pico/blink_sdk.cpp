#include "pico/stdlib.h"

constexpr unsigned led_pin = 16;
int main() {
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    while (true) {
        gpio_put(led_pin, 1);
        sleep_ms(500);
        gpio_put(led_pin, 0);
        sleep_ms(500);
    }
}
