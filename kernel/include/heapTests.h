#ifndef HEAP_TESTS_H
#define HEAP_TESTS_H

#include "heap.h"
#include "printf.h"

struct HeapTestStruct
{
  uint64_t test;
};

void heapTests() {
    // Test 1: Basic allocation
    void* block1 = malloc(256, 8);
    if (block1 != 0) {
        Debug::printf("Test 1 Passed: Allocated 256 bytes.\n");
    } else {
        Debug::printf("Test 1 Failed: Allocation returned null.\n");
    }

    // Test 2: Large allocation within heap size
    void* block2 = malloc(500000);
    if (block2 != 0) {
        Debug::printf("Test 2 Passed: Allocated 500000 bytes.\n");
    } else {
        Debug::printf("Test 2 Failed: Allocation returned null.\n");
    }

    // Test 3: Allocation exceeding available heap space
    void* block3 = malloc(700000);  // This should fail
    if (block3 == 0) {
        Debug::printf("Test 3 Passed: Out-of-memory condition handled.\n");
    } else {
        Debug::printf("Test 3 Failed: Allocation succeeded unexpectedly.\n");
    }

    // Test 4: Testing new keyword
    HeapTestStruct* block4 = new HeapTestStruct();
    if (block4 != 0) {
        Debug::printf("Test 4 Passed: New keyword succeeded.\n");
    } else {
        Debug::printf("Test 4 Failed: New keywork failed.\n");
    }

    // Test 5: Testing allocating all remaining heap space
    size_t remaining_space = HEAP_END - ((size_t)block4 + sizeof(HeapTestStruct));
    void* block5 = malloc(remaining_space);
    if (block5 != 0) {
        Debug::printf("Test 5 Passed: Allocated remaining heap space.\n");
    } else {
        Debug::printf("Test 5 Failed: Allocation returned null.\n");
    }
}

#endif