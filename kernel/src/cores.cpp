#include "cores.h"
#include "definitions.h"
#include "stdint.h"
#include "printf.h"
#include "event_loop.h"


Atomic<int> startedCores = Atomic<int>(0);

uint8_t stack0[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack0_top;
uint8_t* stack0_top = (stack0 + STACK_SIZE);

uint8_t stack1[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack1_top;
uint8_t* stack1_top = (stack1 + STACK_SIZE);

uint8_t stack2[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack2_top;
uint8_t* stack2_top = (stack2 + STACK_SIZE);

uint8_t stack3[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack3_top;
uint8_t* stack3_top = (stack3 + STACK_SIZE);

extern "C" void _start_core1();
extern "C" void _start_core2();
extern "C" void _start_core3();

extern "C" void initCore1()
{
  Debug::printf("Core 1!\n");
  startedCores.add_fetch(1);

  event_loop();
}

extern "C" void initCore2()
{
  Debug::printf("Core 2!\n");
  startedCores.add_fetch(1);

  event_loop();
}

extern "C" void initCore3()
{
  Debug::printf("Core 3!\n");
  startedCores.add_fetch(1);

  event_loop();
}


void bootCores()
{
  startedCores.add_fetch(1);
  // Boot other cores
  uint64_t* core_wakeup_base = (uint64_t*) 216;
  *(core_wakeup_base + 1) = (uint64_t) &_start_core1;
  *(core_wakeup_base + 2) = (uint64_t) &_start_core2;
  *(core_wakeup_base + 3) = (uint64_t) &_start_core3;
}