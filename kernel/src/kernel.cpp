// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "stdint.h"
#include "uart.h"
#include "debug.h"
#include "atomics.h"
#include "event_loop.h"

#define STACK_SIZE 8192

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

extern "C" void kernelMain_core1()
{
  debug_print("Core 1!\n");
}

extern "C" void kernelMain_core2()
{
  debug_print("Core 2!\n");
}

extern "C" void kernelMain_core3()
{
  debug_print("Core 3!\n");
}

extern "C" void kernelMain()
{
  init_uart();

  debug_print("DingOS is Booting!\n");

  // Boot other cores
  uint64_t* core_wakeup_base = (uint64_t*) 216;
  *(core_wakeup_base + 1) = (uint64_t) &_start_core1;
  *(core_wakeup_base + 2) = (uint64_t) &_start_core2;
  *(core_wakeup_base + 3) = (uint64_t) &_start_core3;

  // event_loop_test();

  while(1);
}