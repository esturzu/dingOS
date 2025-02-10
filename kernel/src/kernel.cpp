// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
#include "event_loop.h"
#include "heap.h"
#include "printf.h"
#include "stdint.h"
#include "tester.h"
#include "uart.h"

extern "C" void kernelMain() {
  CRTI::_init();

  heap_init();
  init_event_loop();
  init_uart();

  Debug::printf("DingOS is Booting!\n");

  bootCores();

  // commented out this approach for testing
  // setupTests();

  event_loop();

  while (1);
}