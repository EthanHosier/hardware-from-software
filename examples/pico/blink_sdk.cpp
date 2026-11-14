#include "iostream"
#include "pico/stdlib.h"

constexpr unsigned led_pin = 16;
constexpr uint32_t mask = 1ul << led_pin;

void play_note(int pitch, float duration_secs) {
    assert(pitch <= 100 && pitch >= 0);
    int sleep_micros_per_iter = (100 - pitch) * 10;
    int num_iterations = static_cast<int>(
        duration_secs * 1'000'000.0f / static_cast<float>(sleep_micros_per_iter)
    );

    for (int i = 0; i < num_iterations; i ++) {
        sio_hw->gpio_set = mask;
        sleep_us(sleep_micros_per_iter / 2);
        sio_hw->gpio_clr = mask;
        sleep_us(sleep_micros_per_iter / 2);
    }
}

int main() {
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    while (true) {
        std::cout << "lower pitch" << std::endl;
        play_note(5, 5);

        std::cout << "higher pitch" << std::endl;
        play_note(40, 5);
    }
}