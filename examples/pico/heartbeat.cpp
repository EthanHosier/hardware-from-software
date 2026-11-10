#include <cstdio>
#include "pico/stdlib.h"
int main() {
    stdio_init_all();
    unsigned count = 0;
    while (true) {
        std::printf("hardware-from-software heartbeat v1: %u\n", count++);
        sleep_ms(1000);
    }
}
