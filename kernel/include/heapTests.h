#ifndef HEAP_TESTS_H
#define HEAP_TESTS_H

#include "heap.h"
#include "printf.h"
#include "testFramework.h"

struct HeapTestStruct
{
  uint64_t test;
};

void heapTests() {
    initTests("Heap Tests");
    // Test 1: Basic allocation
    void* block1 = malloc(256, 8);
    testsResult("Basic allocation", block1 != 0);

    // Test 2: Large allocation within heap size
    void* block2 = malloc(500000);
    testsResult("Large allocation within heap size", block2 != 0); 

    // Test 3: Allocation exceeding available heap space
    void* block3 = malloc(700000);  // This should fail
    testsResult("Out-of-memory condition handled", block3 == 0);

    // Test 4: Testing new keyword
    HeapTestStruct* block4 = new HeapTestStruct();
    testsResult("New keyword", block4 != 0);

    // Test 5: Testing allocating all remaining heap space
    size_t remaining_space = HEAP_END - ((size_t)block4 + sizeof(HeapTestStruct));
    void* block5 = malloc(remaining_space);
    testsResult("Allocating all remaining heap space", block5 != 0);
}

#endif