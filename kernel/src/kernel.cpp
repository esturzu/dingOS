#include "kernel.h"

#include "stdint.h"
#include "printf.h"
#include "uart.h"
#include "atomics.h"
#include "event_loop.h"
#include "heap.h"

extern "C" void kernelMain()
{
  init_uart();

  Debug::printf("DingOS is Booting!\n");

  run_heap_tests();

  event_loop_test();

  // while(1);
}