// Include necessary headers
#include "kernel.h"
#include "stdint.h"
#include "printf.h"
#include "uart.h"
#include "atomics.h"
#include "event_loop.h"
#include "heap.h"

// Stack setup for multiple cores
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

// Core-specific entry points for other cores
extern "C" void kernelMain_core1() {
    Debug::printf("Core 1 is active!\n");
    event_loop();
}

extern "C" void kernelMain_core2() {
    Debug::printf("Core 2 is active!\n");
    event_loop();
}

extern "C" void kernelMain_core3() {
    Debug::printf("Core 3 is active!\n");
    event_loop();
}

void testing_heap() {
    void* block1 = malloc(256, 8);
    if (block1 != 0) {
        Debug::printf("Test 1 Passed: Allocated 256 bytes.\n");
    } else {
        Debug::printf("Test 1 Failed: Allocation returned null.\n");
    }
}


// Main entry point for this test
extern "C" void kernelMain() {
    init_uart();
    uint64_t* core_wakeup_base = (uint64_t*) 216;
    *(core_wakeup_base + 1) = (uint64_t) &_start_core1;
    *(core_wakeup_base + 2) = (uint64_t) &_start_core2;
    *(core_wakeup_base + 3) = (uint64_t) &_start_core3;
    Debug::printf("Running test: kernelMain in test_heap.cpp\n");

    // Example heap allocation test
    testing_heap();

}

