#include <array>
#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "device_io.hpp"

namespace {
std::array<uint8_t, 16> published_registers{}, read_snapshot{};
uint8_t register_cursor = 0;
bool has_register_pointer = false, snapshot_captured = false,
     led_update_pending = false;
uint8_t requested_led_state = 0;
void write_le32(std::array<uint8_t, 16>& registers, unsigned offset, uint32_t value) {
    for (unsigned byte_index = 0; byte_index < 4; ++byte_index) {
        registers[offset + byte_index] =
            static_cast<uint8_t>(value >> (8 * byte_index));
    }
}
void on_i2c(i2c_inst_t* bus, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {
        const uint8_t byte = i2c_read_byte_raw(bus);
        if (!has_register_pointer) {
            register_cursor = byte & 15;
            has_register_pointer = true;
        } else {
            if (register_cursor == 2 && byte <= 1) {
                requested_led_state = byte;
                led_update_pending = true;
            }
            register_cursor = (register_cursor + 1) & 15;
        }
        break;
    }
    case I2C_SLAVE_REQUEST:
        // First read after a pointer write/repeated START fixes a whole snapshot.
        if (!snapshot_captured) {
            read_snapshot = published_registers;
            snapshot_captured = true;
        }
        i2c_write_byte_raw(bus, read_snapshot[register_cursor]);
        register_cursor = (register_cursor + 1) & 15;
        break;
    case I2C_SLAVE_FINISH:
        has_register_pointer = false;
        snapshot_captured = false;
        break;
    }
}
} // namespace
int main() {
    Device device;
    published_registers[0] = 0x42;
    published_registers[1] = 1;
    i2c_init(i2c0, 100000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_disable_pulls(4);
    gpio_disable_pulls(5);
    // Pi header SDA/SCL already have external pull-ups. No added Pico pull-ups.
    i2c_slave_init(i2c0, 0x42, on_i2c);
    while (true) {
        device.update_button();
        auto next_registers = published_registers;
        write_le32(next_registers, 4, device.presses);
        write_le32(next_registers, 8, static_cast<uint32_t>(device.temperature()));
        const uint32_t saved_interrupt_state = save_and_disable_interrupts();
        if (led_update_pending) {
            device.led(requested_led_state != 0);
            led_update_pending = false;
        }
        next_registers[2] = device.light;
        published_registers = next_registers;
        restore_interrupts(saved_interrupt_state);
        sleep_ms(1);
    }
}
