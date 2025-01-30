// Citations
// https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
// https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf

#include "uart.h"
#include "stdint.h"
#include "peripherals_mappings.h"

void init_uart ()
{
  volatile uint32_t* UART0_CONTROL_REGISTER = (volatile uint32_t*) (UART0_CONTROL_PHYS);
  
  *UART0_CONTROL_REGISTER = 0x00000000;
}

void uart_putc(char c)
{
  volatile uint32_t* UART0_DATA_REGISTER = (volatile uint32_t*) (UART0_DATA_PHYS);
  volatile uint32_t* UART0_FLAG_REGISTER = (volatile uint32_t*) (UART0_FLAG_PHYS);

  while ((*UART0_FLAG_REGISTER >> 5) & 0x1) {}

  *UART0_DATA_REGISTER = c;
}