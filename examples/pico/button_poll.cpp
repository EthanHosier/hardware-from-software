#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/structs/sio.h"
constexpr unsigned button = 17;
int main() {
    stdio_init_all();
    gpio_init(button); gpio_set_dir(button, GPIO_IN); gpio_disable_pulls(button);
    // External 4.7k pull-up: pressed connects GP17 to ground.
    bool last = (sio_hw->gpio_in & (1u << button)) == 0;
    unsigned transitions = 0;
    while (true) {
        const bool pressed = (sio_hw->gpio_in & (1u << button)) == 0;
        if (pressed != last) {
            last = pressed;
            std::printf("raw transition %u pressed=%d\n", ++transitions, pressed);
        }
        tight_loop_contents();
    }
}
