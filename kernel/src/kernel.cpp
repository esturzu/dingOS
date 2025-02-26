// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "heap.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"

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

  setupTests();

  SystemTimer::setup_timer(0);
  schedule_event([=]() {
    uint64_t last_time = current_time;
    while (true) {
      if (last_time != current_time) {
        debug_printf("Heartbeat: %u\n", current_time);
        last_time = current_time;
      }
    }
  });

  run_page_tests();

  event_loop();

  while (1)
    ;
}
