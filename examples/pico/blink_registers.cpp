#include "pico/stdlib.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/sio.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/pads_bank0.h"
constexpr unsigned led = 16;
constexpr uint32_t mask = 1u << led;
// These SDK structs describe the RP2350 MMIO layout, not ordinary RAM.
extern "C" __attribute__((noinline)) void led_on() { sio_hw->gpio_set = mask; }
extern "C" __attribute__((noinline)) void led_off() { sio_hw->gpio_clr = mask; }
int main() {
    sio_hw->gpio_oe_clr = mask; // Keep output disabled during configuration.
    sio_hw->gpio_clr = mask;    // Select a low initial output value.
    io_bank0_hw->io[led].ctrl = GPIO_FUNC_SIO;
    // Clear isolation/output-disable/pulls; choose 2mA drive, slow slew.
    // IE is unnecessary for a pure output; SCHMITT does not hurt it.
    pads_bank0_hw->io[led] = PADS_BANK0_GPIO0_SCHMITT_BITS;
    sio_hw->gpio_oe_set = mask;
    while (true) { led_on(); sleep_ms(500); led_off(); sleep_ms(500); }
}
