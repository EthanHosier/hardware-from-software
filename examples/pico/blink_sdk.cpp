#include "iostream"
#include "map"
#include "set"
#include "pico/stdlib.h"

constexpr unsigned led_pin = 16;
constexpr uint32_t mask = 1ul << led_pin;
constexpr int ONE_SECOND_IN_NANOS = 1'000'000;


#define C       261.63f
#define C_SHARP 277.18f
#define D_FLAT  277.18f
#define D       293.66f
#define D_SHARP 311.13f
#define E_FLAT  311.13f
#define E       329.63f
#define F       349.23f
#define F_SHARP 369.99f
#define G_FLAT  369.99f
#define G       392.00f
#define G_SHARP 415.30f
#define A_FLAT  415.30f
#define A       440.00f
#define A_SHARP 466.16f
#define B_FLAT  466.16f
#define B       493.88f

void play_note(float freq, float duration_secs = 1) {
    float sleep_nanos = ONE_SECOND_IN_NANOS / freq;
    float num_iterations = duration_secs * ONE_SECOND_IN_NANOS / sleep_nanos;
    int set_sleep = static_cast<int>(sleep_nanos / 2);

    for (auto i = 0; i < static_cast<int>(num_iterations); i ++) {
        sio_hw->gpio_set = mask;
        sleep_us(set_sleep);
        sio_hw->gpio_clr = mask;
        sleep_us(set_sleep);
    }
}

int main() {
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    auto frequencies = std::set{
        C, C_SHARP, D_FLAT, D, D_SHARP, E_FLAT, E, F,
        F_SHARP, G_FLAT, G, G_SHARP, A_FLAT, A, A_SHARP, B_FLAT, B
    };

    while (true) {
        for (auto freq: frequencies) {
            play_note(freq);
        }
    }
}