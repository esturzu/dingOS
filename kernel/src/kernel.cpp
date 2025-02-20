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

  // testing read

  uint32_t startBlock = 0;
  uint32_t blocks = 10;
  uint8_t buffer[blocks * SD::BLOCKSIZE];

  uint32_t res = SD::read(startBlock, blocks, buffer);
  uint32_t width = 4;
  for(int i = 0; i < blocks * SD::BLOCKSIZE; i++) {
    if(i % (SD::BLOCKSIZE) == 0) {
      printf("\nBlock %u\n", i / SD::BLOCKSIZE + startBlock);
    }
    printf("%02x", buffer[i]);
    if(i % 4 == 3){
      printf(" ");
    }
    if(i % (width * 4) == ((width * 4 - 1))) {
      printf("\n");
    }
  }
  if(res == blocks * SD::BLOCKSIZE) {
    printf("Read Success!\n");
  } else {
    printf("Read Failed!\n");
  }

  // setupTests();

  // run_page_tests();

  event_loop();

  while (1);
}