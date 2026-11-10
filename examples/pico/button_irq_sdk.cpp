#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/sync.h"
volatile bool pending=false;
void button_callback(unsigned gpio,uint32_t events) {
    if (gpio==17 && (events & GPIO_IRQ_EDGE_FALL)) pending=true;
}
int main() {
    stdio_init_all(); gpio_init(17); gpio_set_dir(17,GPIO_IN); gpio_disable_pulls(17);
    gpio_set_irq_enabled_with_callback(17,GPIO_IRQ_EDGE_FALL,true,button_callback);
    while (true) {
        const uint32_t saved=save_and_disable_interrupts();
        const bool report=pending; pending=false;
        restore_interrupts(saved);
        if (report) std::puts("falling edge (raw; may bounce or coalesce)");
        sleep_ms(1);
    }
}
