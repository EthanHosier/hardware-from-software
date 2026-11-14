#include "iostream"
#include "map"
#include "set"
#include "vector"
#include "pico/stdlib.h"

constexpr unsigned led_pin = 16;

constexpr uint32_t to_mask(const uint32_t u) {
    return 1ul << u;
}

constexpr uint32_t mask = to_mask(led_pin);
constexpr int ONE_SECOND_IN_MICROS = 1'000'000;


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

#define HAPPY_BIRTHDAY { \
    {G, 0.8f}, {G, 0.2f}, {A, 1.0f}, {G, 1.0f}, {C * 2, 1.0f}, {B, 2.0f}, \
    {G, 0.8f}, {G, 0.2f}, {A, 1.0f}, {G, 1.0f}, {D * 2, 1.0f}, {C * 2, 2.0f}, \
    {G, 0.8f}, {G, 0.2f}, {G * 2, 1.0f}, {E * 2, 1.0f}, {C * 2, 1.0f}, {B, 1.0f}, {A, 2.0f}, \
    {F * 2, 0.8f}, {F * 2, 0.2f}, {E * 2, 1.0f}, {C * 2, 1.0f}, {D * 2, 1.0f}, {C * 2, 2.0f} \
}

#define LUSH_LIFE { \
    {G, 0.5f}, {B_FLAT, 1.0f}, {B_FLAT, 1.5f}, {G, 2.0f}, \
    {G, 1.0f}, {B_FLAT, 1.0f}, {C * 2, 1.0f}, \
    {D * 2, 0.5f}, {B_FLAT, 1.0f}, {G, 1.5f}, {B_FLAT, 2.0f}, \
    {C * 2, 1.0f}, {F, 0.5f}, {G, 0.5f}, {B_FLAT, 0.5f}, {C * 2, 0.5f}, \
    {G, 0.5f}, {B_FLAT, 1.0f}, {B_FLAT, 1.5f}, {G, 2.0f}, \
    {G, 1.0f}, {B_FLAT, 1.0f}, {C * 2, 1.0f}, \
    {D * 2, 0.5f}, {B_FLAT, 1.0f}, {G, 0.5f}, \
    {D * 2, 0.5f}, {C * 2, 1.0f}, {B_FLAT, 0.5f}, \
    \
    {0.0f, 0.5f}, \
    {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 1.0f}, {F * 2, 1.0f}, \
    {F * 2, 1.0f}, {G * 2, 0.5f}, {F * 2, 1.0f}, {D * 2, 0.5f}, {D * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {C * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.0f}, {B_FLAT, 0.5f}, {C * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 2.5f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.5f}, \
    {0.0f, 1.0f}, {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 1.0f}, {F * 2, 1.0f}, \
    {F * 2, 1.0f}, {G * 2, 0.5f}, {F * 2, 1.0f}, {D * 2, 0.5f}, {D * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {C * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.0f}, {B_FLAT, 0.5f}, {C * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 2.5f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.5f}, \
    {0.0f, 4.0f}, \
    \
    {0.0f, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 2.0f}, {B_FLAT, 0.5f}, {C * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 4.0f}, \
    {0.0f, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 2.0f}, {B_FLAT, 0.5f}, {C * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 0.5f}, {D * 2, 3.0f}, \
    {0.0f, 4.0f}, \
    {0.0f, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 2.0f}, {B_FLAT, 0.5f}, {C * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 4.0f}, \
    {0.0f, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {G, 2.0f}, {B_FLAT, 0.5f}, {C * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 0.5f}, {D * 2, 3.0f}, \
    \
    {0.0f, 0.5f}, \
    {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 1.0f}, {F * 2, 1.0f}, \
    {F * 2, 1.0f}, {G * 2, 0.5f}, {F * 2, 1.0f}, {D * 2, 0.5f}, {D * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {C * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.0f}, {B_FLAT, 0.5f}, {C * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 2.5f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.5f}, \
    {0.0f, 1.0f}, {F * 2, 0.5f}, {F * 2, 0.5f}, {F * 2, 1.0f}, {F * 2, 1.0f}, \
    {F * 2, 1.0f}, {G * 2, 0.5f}, {F * 2, 1.0f}, {D * 2, 0.5f}, {D * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {C * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.0f}, {B_FLAT, 0.5f}, {C * 2, 2.0f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 2.5f}, \
    {D * 2, 0.5f}, {D * 2, 0.5f}, {D * 2, 1.0f}, {D * 2, 1.0f}, \
    {D * 2, 1.0f}, {D * 2, 1.0f}, {D * 2, 0.5f}, {C * 2, 1.5f}, \
    {0.0f, 4.0f} \
}

struct Note {
    float frequency;
    float duration_s;
};

constexpr uint32_t lights[]{13,14,15};

void flash_lights(bool b) {
    for (auto l: lights) {
        if (b ^ (l % 2 != 0)) {
            sio_hw->gpio_set = to_mask(l);
        } else {
            sio_hw->gpio_clr = to_mask(l);
        }
    }
}

void play_note(float freq, float duration_secs = 1) {
    if (freq == 0) { sleep_ms(static_cast<unsigned>(duration_secs * 1000)); return;};
    float sleep_micros = ONE_SECOND_IN_MICROS / freq;
    int set_sleep = static_cast<int>(sleep_micros / 2);

    bool shouldFlash = false;

    for (auto i = 0; i < static_cast<int>(freq*duration_secs); i ++) {
        if (i % 100 == 0) {
            flash_lights(shouldFlash);
            shouldFlash = !shouldFlash;
        }

        sio_hw->gpio_set = mask;
        sleep_us(set_sleep);

        sio_hw->gpio_clr = mask;
        sleep_us(set_sleep);
    }

    sleep_us(200);
}



int main() {
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    for (auto l: lights) {
        gpio_init(l);
        gpio_set_dir(l, GPIO_OUT);
    }

    auto frequencies = std::set{
        C, C_SHARP, D_FLAT, D, D_SHARP, E_FLAT, E, F,
        F_SHARP, G_FLAT, G, G_SHARP, A_FLAT, A, A_SHARP, B_FLAT, B
    };

    auto song{std::vector<Note> {LUSH_LIFE}};

    while (true) {
        for (auto [freq, dur]: song) {
            play_note(freq, dur);
        }
    }
}
