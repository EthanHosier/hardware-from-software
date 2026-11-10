#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bme280_common.hpp"
struct Bus {
    static constexpr uint8_t address=0x77; // Adafruit2652 default; SDO low selects0x76.
    bool read(uint8_t reg,uint8_t* data,unsigned len) {
        return i2c_write_timeout_us(i2c0,address,&reg,1,true,100000)==1 &&
            i2c_read_timeout_us(i2c0,address,data,len,false,100000)==static_cast<int>(len);
    }
    bool write(uint8_t reg,uint8_t value) {
        uint8_t b[]{reg,value};
        return i2c_write_timeout_us(i2c0,address,b,2,false,100000)==2;
    }
};
int main() {
    stdio_init_all();
    i2c_init(i2c0,100000); gpio_set_function(4,GPIO_FUNC_I2C); gpio_set_function(5,GPIO_FUNC_I2C);
    gpio_disable_pulls(4); gpio_disable_pulls(5);
    // Adafruit board supplies the external I2C pullups when powered at3.3V.
    sleep_ms(2000); Bus bus; temperature_demo(bus);
    while (true) sleep_ms(1000); // Keep diagnostic USB alive after an error.
}
