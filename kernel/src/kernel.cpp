// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
#include "event_loop.h"
#include "heap.h"
#include "machine.h"
#include "printf.h"
#include "stdint.h"
#include "tester.h"
#include "uart.h"

extern "C" void kernelMain() {
  // Handled uart Init
  CRTI::_init();

  Debug::printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  init_event_loop();

  Debug::printf("DingOS is Booting!\n");
  Debug::printf("Core %d! %s\n", SMP::whichCore(), STRING_EL(get_CurrentEL()));

  SMP::bootCores();

  setupTests();

  event_loop();

  while (1);
}