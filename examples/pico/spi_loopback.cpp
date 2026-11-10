#include <array>
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/spi.h"
int main() {
    stdio_init_all(); spi_init(spi0, 100000);
    gpio_set_function(18, GPIO_FUNC_SPI); gpio_set_function(19, GPIO_FUNC_SPI);
    gpio_set_function(20, GPIO_FUNC_SPI);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    const std::array<uint8_t,4> tx{0x00,0x55,0xaa,0xff};
    while (true) {
        std::array<uint8_t,4> rx{};
        spi_write_read_blocking(spi0, tx.data(), rx.data(), tx.size());
        std::printf("GP19->GP20 loopback: %s [%02x %02x %02x %02x]\n",
            tx == rx ? "pass" : "FAIL", rx[0],rx[1],rx[2],rx[3]);
        sleep_ms(1000);
    }
}
