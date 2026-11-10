#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/structs/io_bank0.h"
constexpr unsigned button = 17, led = 16;
constexpr unsigned word = button / 8, shift = 4 * (button % 8);
constexpr uint32_t edges = (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL) << shift;
volatile bool changed = false; // Accesses are paired with an IRQ-masked handoff.
extern "C" void gpio_isr() {
    const uint32_t pending = io_bank0_hw->proc0_irq_ctrl.ints[word] & edges;
    io_bank0_hw->intr[word] = pending; // W1C edge latches; do not RMW.
    if (pending) changed = true;
}
int main() {
    stdio_init_all();
    gpio_init(led); gpio_set_dir(led, GPIO_OUT);
    gpio_init(button); gpio_set_dir(button, GPIO_IN); gpio_disable_pulls(button);
    irq_set_exclusive_handler(IO_IRQ_BANK0, gpio_isr);
    io_bank0_hw->intr[word] = edges;
    hw_set_bits(&io_bank0_hw->proc0_irq_ctrl.inte[word], edges);
    irq_set_enabled(IO_IRQ_BANK0, true);
    bool stable = gpio_get(button), candidate = stable, light = false;
    uint64_t since = time_us_64();
    unsigned presses = 0;
    while (true) {
        const uint32_t saved = save_and_disable_interrupts();
        const bool edge_seen = changed; changed = false;
        restore_interrupts(saved);
        const bool raw = gpio_get(button);
        const uint64_t now = time_us_64();
        if (edge_seen || raw != candidate) { candidate = raw; since = now; }
        if (candidate != stable && now - since >= 20000) {
            stable = candidate;
            if (!stable) {
                gpio_put(led, light = !light);
                std::printf("accepted press %u led=%d\n", ++presses, light);
            }
        }
        sleep_ms(1);
    }
}
