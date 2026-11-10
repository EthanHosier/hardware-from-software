#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "bme280_common.hpp"
struct Bus {
    static constexpr unsigned cs=21;
    bool read(uint8_t reg,uint8_t* data,unsigned len) {
        reg |= 0x80;
        gpio_put(cs,0);
        const int a=spi_write_blocking(spi0,&reg,1);
        const int b=spi_read_blocking(spi0,0,data,len);
        gpio_put(cs,1); return a==1 && b==static_cast<int>(len);
    }
    bool write(uint8_t reg,uint8_t value) {
        const uint8_t bytes[]{static_cast<uint8_t>(reg & 0x7f),value};
        gpio_put(cs,0); const int n=spi_write_blocking(spi0,bytes,2); gpio_put(cs,1); return n==2;
    }
};
int main() {
    stdio_init_all();
    gpio_init(Bus::cs); gpio_put(Bus::cs,1); gpio_set_dir(Bus::cs,GPIO_OUT);
    spi_init(spi0,100000); spi_set_format(spi0,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
    gpio_set_function(18,GPIO_FUNC_SPI); gpio_set_function(19,GPIO_FUNC_SPI); gpio_set_function(20,GPIO_FUNC_SPI);
    sleep_ms(2000); Bus bus; temperature_demo(bus);
    while (true) sleep_ms(1000);
}
