// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "bfs.h"
#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "framebuffer.h"
#include "heap.h"
#include "interrupts.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"
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

  run_page_tests();

  SD::init();

  fs_init();

  // setupTests();

  // event_loop();

  // Request a framebuffer at 800x600x32
  FrameBufferInfo* fb = framebuffer_init(640, 480, 32);
  if (!fb) {
    debug_printf("Failed to init framebuffer!\n");
    while (true) { /* spin */
    }
  }

  // Fill the screen with a color
  framebuffer_fill(fb, 0xFFFF00);

  debug_printf("FILLED THE FRAME BUFFER!\n");

  while (1);
}
