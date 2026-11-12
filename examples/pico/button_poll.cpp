#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/structs/sio.h"

constexpr unsigned button_pin = 17;
int main() {
    stdio_init_all();
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_disable_pulls(button_pin);
    // External 4.7k pull-up: pressed connects GP17 to ground.
    bool previous_pressed = (sio_hw->gpio_in & (1u << button_pin)) == 0;
    unsigned transition_count = 0;
    while (true) {
        const bool pressed = (sio_hw->gpio_in & (1u << button_pin)) == 0;
        if (pressed != previous_pressed) {
            previous_pressed = pressed;
            std::printf("raw transition %u pressed=%d\n", ++transition_count, pressed);
        }
        tight_loop_contents();
    }
}
