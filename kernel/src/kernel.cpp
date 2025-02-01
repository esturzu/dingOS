#include "kernel.h"

#include "stdint.h"
#include "printf.h"
#include "uart.h"
#include "atomics.h"
#include "event_loop.h"

extern "C" void kernelMain()
{
  init_uart();

  Debug::printf("Hello %s %d %x\n", "Hi!", 7, 11);

  event_loop_test();

  // while(1);
}