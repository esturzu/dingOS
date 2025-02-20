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
#include "bfs.h"

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
  fs_init();

  // Simple BFS test
  fs_create("hello.txt", 12);
  fs_write("hello.txt", "Hello, BFS!", 12);
  
  char buffer[32] = {0};
  fs_read("hello.txt", buffer);
  printf("Read from file: %s\n", buffer);

  fs_list(); 

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

  while (1);
}