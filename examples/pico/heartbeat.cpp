#include <cstdio>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    unsigned heartbeat_count = 0;
    while (true) {
        std::printf("hardware-from-software heartbeat v1: %u\n", heartbeat_count++);
        sleep_ms(1000);
    }
}
