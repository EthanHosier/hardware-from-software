#include "pico/stdlib.h"
#include "hardware/uart.h"
int main() {
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); gpio_set_function(1, GPIO_FUNC_UART);
    uart_set_hw_flow(uart0, false, false); uart_set_format(uart0,8,1,UART_PARITY_NONE);
    uart_set_translate_crlf(uart0, false);
    while (true) if (uart_is_readable(uart0)) uart_putc_raw(uart0, uart_getc(uart0));
}
