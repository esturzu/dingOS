// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
#include "event_loop.h"
#include "heap.h"
#include "printf.h"
#include "stdint.h"
#include "uart.h"
#include "testFramework.h"

// Struct for testing the `new` keyword
struct HeapTestStruct {
    uint8_t test;
};

// Function containing all the heap test logic
void runHeapTests() {
    initTests("Heap Tests");

    // --- Setup phase ---
    uint64_t* block = (uint64_t*)malloc(256);
    for (int i = 0; i < 32; i++) {
        block[i] = (uint64_t)malloc(16);
    }

    for (int i = 1; i < 32; i *= 2) {
        free((void*)block[i]);
    }

    for (int i = 0; i < 32; i++) {
        if (i == 0 || (i & (i - 1)) != 0) {
            free((void*)block[i]);
        }
    }

    // --- Test 1: Basic allocation ---
    void* block1 = malloc(256);
    testsResult("Basic allocation", block1 != 0);

    // --- Test 2: Large allocation within heap size ---
    char* block2 = (char*)malloc(0x10000000);
    testsResult("Large allocation within heap size", block2 != 0);

    // --- Test 3: Allocation exceeding available heap space ---
    void* block3 = malloc(0x20000000);  // This should fail
    testsResult("Allocation exceeding available heap space", block3 == 0);

    // --- Test 4: Testing the `new` keyword ---
    HeapTestStruct* block4 = new HeapTestStruct();
    testsResult("Testing new keyword", block4 != 0);
}

extern "C" void kernelMain() {
  CRTI::_init();

  heap_init();
  init_event_loop();
  init_uart();

  Debug::printf("DingOS is Booting!\n");

  bootCores();
  runHeapTests();
  event_loop();

  while (1);
}


