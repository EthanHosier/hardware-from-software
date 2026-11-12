#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/structs/io_bank0.h"

constexpr unsigned button_pin = 17, led_pin = 16;
constexpr unsigned interrupt_register_index = button_pin / 8,
                   interrupt_bit_shift = 4 * (button_pin % 8);
constexpr uint32_t edge_interrupt_mask = (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL)
                                         << interrupt_bit_shift;
volatile bool edge_pending = false; // Accesses are paired with an IRQ-masked handoff.
extern "C" void gpio_isr() {
    const uint32_t pending =
        io_bank0_hw->proc0_irq_ctrl.ints[interrupt_register_index] &
        edge_interrupt_mask;
    io_bank0_hw->intr[interrupt_register_index] =
        pending; // W1C edge latches; do not RMW.
    if (pending) {
        edge_pending = true;
    }
}

int main() {
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_disable_pulls(button_pin);
    irq_set_exclusive_handler(IO_IRQ_BANK0, gpio_isr);
    io_bank0_hw->intr[interrupt_register_index] = edge_interrupt_mask;
    hw_set_bits(&io_bank0_hw->proc0_irq_ctrl.inte[interrupt_register_index],
                edge_interrupt_mask);
    irq_set_enabled(IO_IRQ_BANK0, true);
    bool stable_level = gpio_get(button_pin), candidate_level = stable_level,
         led_is_on = false;
    uint64_t candidate_since_us = time_us_64();
    unsigned press_count = 0;
    while (true) {
        const uint32_t saved_interrupt_state = save_and_disable_interrupts();
        const bool edge_seen = edge_pending;
        edge_pending = false;
        restore_interrupts(saved_interrupt_state);
        const bool raw_level = gpio_get(button_pin);
        const uint64_t now_us = time_us_64();
        if (edge_seen || raw_level != candidate_level) {
            candidate_level = raw_level;
            candidate_since_us = now_us;
        }
        if (candidate_level != stable_level && now_us - candidate_since_us >= 20000) {
            stable_level = candidate_level;
            if (!stable_level) {
                gpio_put(led_pin, led_is_on = !led_is_on);
                std::printf("accepted press %u led=%d\n", ++press_count, led_is_on);
            }
        }
        sleep_ms(1);
    }
}
