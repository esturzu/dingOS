#include "kernel.h"

#include "stdint.h"
#include "uart.h"
#include "debug.h"

void kernelMain()
{
  init_uart();

  debug_print("DingOS is Booting!\n");

	while (1) {}
}