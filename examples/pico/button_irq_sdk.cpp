#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/sync.h"

volatile bool edge_pending = false;
void button_callback(unsigned gpio_pin, uint32_t event_mask) {
    if (gpio_pin == 17 && (event_mask & GPIO_IRQ_EDGE_FALL)) {
        edge_pending = true;
    }
}

int main() {
    stdio_init_all();
    gpio_init(17);
    gpio_set_dir(17, GPIO_IN);
    gpio_disable_pulls(17);
    gpio_set_irq_enabled_with_callback(17, GPIO_IRQ_EDGE_FALL, true, button_callback);
    while (true) {
        const uint32_t saved_interrupt_state = save_and_disable_interrupts();
        const bool should_report = edge_pending;
        edge_pending = false;
        restore_interrupts(saved_interrupt_state);
        if (should_report) {
            std::puts("falling edge (raw; may bounce or coalesce)");
        }
        sleep_ms(1);
    }
}
