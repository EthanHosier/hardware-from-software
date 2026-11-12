#include <array>
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/spi.h"

int main() {
    stdio_init_all();
    spi_init(spi0, 100000);
    gpio_set_function(18, GPIO_FUNC_SPI);
    gpio_set_function(19, GPIO_FUNC_SPI);
    gpio_set_function(20, GPIO_FUNC_SPI);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    const std::array<uint8_t, 4> transmitted_bytes{0x00, 0x55, 0xaa, 0xff};
    while (true) {
        std::array<uint8_t, 4> received_bytes{};
        spi_write_read_blocking(spi0, transmitted_bytes.data(), received_bytes.data(),
                                transmitted_bytes.size());
        std::printf("GP19->GP20 loopback: %s [%02x %02x %02x %02x]\n",
                    transmitted_bytes == received_bytes ? "pass" : "FAIL",
                    received_bytes[0], received_bytes[1], received_bytes[2],
                    received_bytes[3]);
        sleep_ms(1000);
    }
}
