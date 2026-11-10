#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "device_io.hpp"
#include "../common/protocol.hpp"
int main() {
    stdio_init_all(); Device device;
    uart_init(uart0,115200);
    gpio_set_function(0,GPIO_FUNC_UART); gpio_set_function(1,GPIO_FUNC_UART);
    uart_set_hw_flow(uart0,false,false); uart_set_format(uart0,8,1,UART_PARITY_NONE);
    uart_set_translate_crlf(uart0,false);
    hfs::Line line;
    while (true) {
        device.update_button();
        // Bound UART work per pass so sustained input does not starve polling.
        for (unsigned budget=0; budget<32 && uart_is_readable(uart0); ++budget) {
            const auto result = line.feed(uart_getc(uart0));
            if (result == hfs::Line::Result::none) continue;
            hfs::Request request;
            if (result == hfs::Line::Result::overflow || !hfs::parse(line.text(),request)) {
                uart_puts(uart0,"0 ERR malformed\n"); continue;
            }
            long long value = 0;
            switch (request.op) {
            case hfs::Op::ping: value=1; break;
            case hfs::Op::led: device.led(request.led); value=device.light; break;
            case hfs::Op::count: value=device.presses; break;
            case hfs::Op::temp: value=device.temperature(); break;
            }
            char reply[64];
            std::snprintf(reply,sizeof(reply),"%lu OK %lld\n",
                static_cast<unsigned long>(request.id),value);
            uart_puts(uart0,reply);
        }
        tight_loop_contents();
    }
}
