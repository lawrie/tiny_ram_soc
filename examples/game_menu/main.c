#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>

#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)

void main() {

  reg_uart_clkdiv = 139;

  print("Hello World!\n");
}

