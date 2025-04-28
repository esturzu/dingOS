#ifndef HEAP_TESTS_H
#define HEAP_TESTS_H

#include "heap.h"
#include "printf.h"
#include "testFramework.h"

struct HeapTestStruct {
  uint8_t test;
};

void heapTests() {
  initTests("Heap Tests");

  // Setup
  uint64_t* block = (uint64_t*)malloc(256);
  for (int i = 0; i < 32; i++) {
    printf("Calling Malloc %d\n", i);
    block[i] = (uint64_t)malloc(16);
  }
  for (int i = 1; i < 32; i *= 2) {
    free((void*)block[i]);
  }
  for (int i = 0; i < 32; i++) {
    if (i == 0 || i & (i - 1) != 0) {
      free((void*)block[i]);
    }
  }
  
  // Test 1: Basic allocation
  void* block1 = malloc(256, 8);
  testsResult("Basic allocation", block1 != 0);

  // Test 2: Large allocation within heap size
  char* block2 = (char*)malloc(0x8000000);
  testsResult("Large allocation within heap size", block2 != 0);

  // Test 3: Allocation exceeding available heap space
  void* block3 = malloc(0x20000000);  // This should fail
  testsResult("Allocation exceeding available heap space", block3 == 0);

  // Test 4: Testing new keyword
  HeapTestStruct* block4 = new HeapTestStruct();
  testsResult("Testing new keyword", block4 != 0);

  delete block4;
  testsResult("Testing delete keyword", ((long*) block4)[-1] > 0);
}

#endif
