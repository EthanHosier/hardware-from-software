#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "bme280_common.hpp"

struct Bus {
    static constexpr unsigned chip_select_pin = 21;
    bool read(uint8_t register_address, uint8_t* buffer, unsigned byte_count) {
        register_address |= 0x80;
        gpio_put(chip_select_pin, 0);
        const int address_bytes_written =
            spi_write_blocking(spi0, &register_address, 1);
        const int data_bytes_read = spi_read_blocking(spi0, 0, buffer, byte_count);
        gpio_put(chip_select_pin, 1);
        return address_bytes_written == 1 &&
               data_bytes_read == static_cast<int>(byte_count);
    }
    bool write(uint8_t register_address, uint8_t value) {
        const uint8_t bytes[]{static_cast<uint8_t>(register_address & 0x7f), value};
        gpio_put(chip_select_pin, 0);
        const int bytes_written = spi_write_blocking(spi0, bytes, 2);
        gpio_put(chip_select_pin, 1);
        return bytes_written == 2;
    }
};
int main() {
    stdio_init_all();
    gpio_init(Bus::chip_select_pin);
    gpio_put(Bus::chip_select_pin, 1);
    gpio_set_dir(Bus::chip_select_pin, GPIO_OUT);
    spi_init(spi0, 100000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(18, GPIO_FUNC_SPI);
    gpio_set_function(19, GPIO_FUNC_SPI);
    gpio_set_function(20, GPIO_FUNC_SPI);
    sleep_ms(2000);
    Bus bus;
    temperature_demo(bus);
    while (true) {
        sleep_ms(1000);
    }
}
