#include "kernel.h"

#include "stdint.h"
#include "uart.h"

void kernelMain()
{
  init_uart();

  uart_putc('h');
  uart_putc('i');
  uart_putc('!');

	while (1) {}
}