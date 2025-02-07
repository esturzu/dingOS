#include "kernel.h"

#include "stdint.h"
#include "uart.h"
#include "debug.h"
#include "atomics.h"
#include "event_loop.h"
#include "heap.h"

extern "C" void kernelMain()
{
  init_uart();

  debug_print("DingOS is Booting!\n");

  heap_init();

  debug_print("Heap initialized.\n");

  run_heap_tests();

  event_loop_test();

  while(1);
}