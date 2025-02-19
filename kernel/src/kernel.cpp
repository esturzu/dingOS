// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
#include "event_loop.h"
#include "heap.h"
#include "interrupts.h"
#include "crti.h"
#include "physmem.h"
#include "machine.h"
#include "printf.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"
#include "uart.h"
#include "sd.h"

extern "C" void kernelMain() {
  // Handled uart Init
  CRTI::_init();

  debug_printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  PhysMem::page_init();
  init_event_loop();

  printf("DingOS is Booting!\n");
  debug_printf("Core %d! %s\n", SMP::whichCore(), STRING_EL(get_CurrentEL()));

  SMP::bootCores();
  
  SD::init();

  setupTests();

  run_page_tests();

  event_loop();

  while (1);
}