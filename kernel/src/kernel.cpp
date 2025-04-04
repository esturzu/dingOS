// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "local_timer.h"
#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "heap.h"
#include "interrupts.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "process.h"
#include "sd.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"
#include "bfs.h"
#include "vmm.h"

extern "C" void kernelMain() {
  // Handled uart Init
  PhysMem::page_init();

  CRTI::_init();

  VMM::init();

  printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  init_event_loop();

  printf("DingOS is Booting!\n");
  debug_printf("Core %d! %s\n", SMP::whichCore(), STRING_EL(get_CurrentEL()));

  SMP::bootCores();

  LocalTimer::setup_timer();

  // run_page_tests();

  // SD::init();

  // fs_init();

  // setupTests();

  printf("XD\n");

  schedule_event([]{
    printf("Here\n");
    Process* proc = new Process();
    printf(":)\n");
    proc->run();
  });

  event_loop();

  while (1)
    ;
}
