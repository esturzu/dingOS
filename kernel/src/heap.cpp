#include "stdint.h"
#include "heap.h"
#include "printf.h"

extern "C" char _end; 
#define HEAP_START ((size_t)&_end)
#define HEAP_SIZE  0x100000  // 1 MB heap size

static size_t current_heap = HEAP_START;
static const size_t heap_end = HEAP_START + HEAP_SIZE;

size_t align_up(size_t addr, size_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

extern "C" void* malloc(size_t size, size_t alignment) {
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

extern "C" void free(void* pointer) {
    // haven't done it lol
}


struct HeapTestStruct
{
  uint64_t test;
};

void run_heap_tests() {
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
    size_t remaining_space = heap_end - current_heap;
    void* block5 = malloc(remaining_space);
    if (block5 != 0) {
        Debug::printf("Test 5 Passed: Allocated remaining heap space.\n");
    } else {
        Debug::printf("Test 5 Failed: Allocation returned null.\n");
    }
}

// C++ Operators
// Citations
// https://en.cppreference.com/w/cpp/memory/new/operator_new
// https://en.cppreference.com/w/cpp/memory/new/operator_delete

void* operator new(size_t count)
{
  return malloc(count, 8);
}

void* operator new[](size_t count)
{
  return malloc(count, 8);
}

void* operator new(size_t count, align_val_t al)
{
  return malloc(count, al);
}

void* operator new[](size_t count, align_val_t al)
{
  return malloc(count, al);
}

void operator delete(void* ptr) noexcept
{
  free(ptr);
}

void operator delete[](void* ptr) noexcept
{
  free(ptr);
}