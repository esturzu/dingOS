// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "stdint.h"
#include "printf.h"
#include "uart.h"
#include "atomics.h"
#include "event_loop.h"
#include "heap.h"

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
  Debug::printf("Core 1!\n");

  event_loop();
}

extern "C" void kernelMain_core2()
{
  Debug::printf("Core 2!\n");

  event_loop();
}

extern "C" void kernelMain_core3()
{
  Debug::printf("Core 3!\n");

  event_loop();
}

extern "C" void kernelMain()
{
    heap_init();
    init_event_loop();
    init_uart();

    Debug::printf("DingOS is Booting!\n");
   
    // Boot other cores
    uint64_t* core_wakeup_base = (uint64_t*) 216;
    *(core_wakeup_base + 1) = (uint64_t) &_start_core1;
    *(core_wakeup_base + 2) = (uint64_t) &_start_core2;
    *(core_wakeup_base + 3) = (uint64_t) &_start_core3;

    for (int i = 0; i < 10; i++)
    {
    schedule_event([i] {
        Debug::printf("Event #%d!\n", i);
    });
    }

    run_heap_tests();

    event_loop();

    while(1);
}