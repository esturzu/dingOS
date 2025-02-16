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
#include "system_timer.h"
#include "interrupts.h"

extern "C" void kernelMain() {
  // Handled uart Init
  CRTI::_init();

  Debug::printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  init_event_loop();

  Debug::printf("DingOS is Booting!\n");

  bootCores();

  setupTests();

  SystemTimer::setup_timer(0);
  schedule_event([=](){
    uint64_t last_time = current_time;
    while (true)
    {
      if (last_time != current_time)
      {
        Debug::printf("Heartbeat: %u\n", current_time);
        last_time = current_time;
      }
    }
  });

  event_loop();

  while (1);
}