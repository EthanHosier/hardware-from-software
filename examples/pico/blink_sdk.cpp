#include "iostream"
#include "pico/stdlib.h"

constexpr unsigned led_pin = 16;
int main() {
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    uint32_t mask = 1ul << led_pin;
    while (true) {
        std::cout << "Led pin mmio!" << std::endl;
        // gpio_put(led_pin, 1);
        sio_hw->gpio_set = mask;
        sleep_ms(500);
        // gpio_put(led_pin, 0);
        sio_hw->gpio_clr = mask;
        sleep_ms(500);
    }
}
