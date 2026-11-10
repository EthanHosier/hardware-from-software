#include "pico/stdlib.h"
constexpr unsigned led = 16;
int main() {
    gpio_init(led); gpio_set_dir(led, GPIO_OUT);
    while (true) {
        gpio_put(led, 1); sleep_ms(500);
        gpio_put(led, 0); sleep_ms(500);
    }
}
