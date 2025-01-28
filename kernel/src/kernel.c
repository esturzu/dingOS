#include "kernel.h"

#include "stdint.h"
#include "uart.h"
#include "debug.h"
#include "atomics.h"



void kernelMain()
{
  init_uart();

  debug_print("DingOS is Booting!\n");


  basic_test_atomics();

  // while(1){};
}