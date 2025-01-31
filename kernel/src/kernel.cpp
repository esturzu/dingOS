#include "kernel.h"

#include "stdint.h"
#include "uart.h"
#include "debug.h"
#include "atomics.h"
#include "event_loop.h"

extern "C" void kernelMain()
{
  init_uart();

  debug_print("DingOS is Booting!\n");


  basic_test_atomics();

  event_loop_test();

  // while(1);
}