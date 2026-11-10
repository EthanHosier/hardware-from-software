#include <array>
#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "device_io.hpp"
namespace {
std::array<uint8_t, 16> published{}, snapshot{};
uint8_t cursor = 0;
bool have_pointer = false, snapshotted = false, led_dirty = false;
uint8_t wanted_led = 0;
void put32(std::array<uint8_t,16>& a, unsigned at, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) a[at+i] = static_cast<uint8_t>(value >> (8*i));
}
void on_i2c(i2c_inst_t* bus, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {
        const uint8_t byte = i2c_read_byte_raw(bus);
        if (!have_pointer) { cursor = byte & 15; have_pointer = true; }
        else {
            if (cursor == 2 && byte <= 1) { wanted_led = byte; led_dirty = true; }
            cursor = (cursor + 1) & 15;
        }
        break;
    }
    case I2C_SLAVE_REQUEST:
        // First read after a pointer write/repeated START fixes a whole snapshot.
        if (!snapshotted) { snapshot = published; snapshotted = true; }
        i2c_write_byte_raw(bus, snapshot[cursor]); cursor = (cursor + 1) & 15;
        break;
    case I2C_SLAVE_FINISH:
        have_pointer = false; snapshotted = false; break;
    }
}
}
int main() {
    Device device;
    published[0] = 0x42; published[1] = 1;
    i2c_init(i2c0, 100000);
    gpio_set_function(4, GPIO_FUNC_I2C); gpio_set_function(5, GPIO_FUNC_I2C);
    // Pi header SDA/SCL already have external pull-ups. No added Pico pull-ups.
    i2c_slave_init(i2c0, 0x42, on_i2c);
    while (true) {
        device.update_button();
        auto next = published;
        put32(next, 4, device.presses);
        put32(next, 8, static_cast<uint32_t>(device.temperature()));
        const uint32_t saved = save_and_disable_interrupts();
        if (led_dirty) { device.led(wanted_led != 0); led_dirty = false; }
        next[2] = device.light; published = next;
        restore_interrupts(saved);
        sleep_ms(1);
    }
}
