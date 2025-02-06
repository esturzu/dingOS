// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "stdint.h"
#include "printf.h"
#include "uart.h"
#include "atomics.h"
#include "event_loop.h"
#include "heap.h"
#include "crti.h"
#include "tester.h"
#include "definitions.h"
#include "cores.h"

extern "C" void kernelMain()
{
  CRTI::_init();

  heap_init();
  init_event_loop();
  init_uart();

  Debug::printf("DingOS is Booting!\n");

  bootCores();
  
  setupTests();

  event_loop();

  while(1);
}