#pragma once
#include "pico/stdlib.h"
#include "hardware/adc.h"
struct Device {
    bool light = false, stable = true, candidate = true;
    uint32_t presses = 0;
    uint64_t since = 0;
    Device() {
        gpio_init(16); gpio_set_dir(16, GPIO_OUT);
        gpio_init(17); gpio_set_dir(17, GPIO_IN); gpio_disable_pulls(17);
        stable = candidate = gpio_get(17); since = time_us_64();
        adc_init(); adc_set_temp_sensor_enabled(true); adc_select_input(4);
    }
    void led(bool value) { light = value; gpio_put(16, value); }
    void update_button() {
        const bool raw = gpio_get(17); const uint64_t now = time_us_64();
        if (raw != candidate) { candidate = raw; since = now; }
        if (candidate != stable && now - since >= 20000) {
            stable = candidate; if (!stable) ++presses;
        }
    }
    int32_t temperature() {
        const float voltage = adc_read() * (3.3f / 4096.0f);
        return static_cast<int32_t>((27.0f - (voltage - 0.706f) / 0.001721f) * 1000.0f);
    }
};
