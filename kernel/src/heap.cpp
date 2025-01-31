#include "stdint.h"
#include "heap.h"
#include "debug.h"

extern "C" char _end; 
#define HEAP_START ((size_t)&_end)
#define HEAP_SIZE  0x100000  // 1 MB heap size

static size_t current_heap = HEAP_START;
static const size_t heap_end = HEAP_START + HEAP_SIZE;

size_t align_up(size_t addr, size_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

void* malloc(size_t size, size_t alignment) {
    // defaults to align to 4 bytes (word thingy from gheith though i am not sure)
    size_t aligned_heap = align_up(current_heap, alignment);

    // check if out of memory
    if (aligned_heap + size > heap_end) {
        return 0;
    }

    void* allocated = (void*)aligned_heap;
    current_heap = aligned_heap + size;
    return allocated;
}

void free(void* pointer) {
    // haven't done it lol
}


void run_heap_tests() {
    // Test 1: Basic allocation
    void* block1 = malloc(256, 8);
    if (block1 != 0) {
        debug_print("Test 1 Passed: Allocated 256 bytes.\n");
    } else {
        debug_print("Test 1 Failed: Allocation returned null.\n");
    }

    // Test 2: Large allocation within heap size
    void* block2 = malloc(500000);
    if (block2 != 0) {
        debug_print("Test 2 Passed: Allocated 500000 bytes.\n");
    } else {
        debug_print("Test 2 Failed: Allocation returned null.\n");
    }

    // Test 3: Allocation exceeding available heap space
    void* block3 = malloc(700000);  // This should fail
    if (block3 == 0) {
        debug_print("Test 3 Passed: Out-of-memory condition handled.\n");
    } else {
        debug_print("Test 3 Failed: Allocation succeeded unexpectedly.\n");
    }

    size_t remaining_space = heap_end - current_heap;
    void* block4 = malloc(remaining_space);
    if (block4 != 0) {
        debug_print("Test 4 Passed: Allocated remaining heap space.\n");
    } else {
        debug_print("Test 4 Failed: Allocation returned null.\n");
    }
}