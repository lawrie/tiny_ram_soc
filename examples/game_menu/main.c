#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>

#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)
#define reg_leds (*(volatile uint32_t*)0x03000000)

void main() {

  reg_uart_clkdiv = 139;

  print("Hello World!\n");

    // blink the user LED
    uint32_t led_timer = 0;
       
    while (1) {
        reg_leds = led_timer >> 16;
        led_timer = led_timer + 1;
    } 
}

