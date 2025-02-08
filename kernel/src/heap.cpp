

    

#include "stdint.h"
#include "heap.h"
#include "printf.h"
#include "atomics.h"

extern "C" char _end; 
extern "C" char _heap_start;
extern "C" char _heap_end;

static size_t current_heap = (size_t) &_heap_start;
static const size_t heap_end = (size_t) &_heap_end;

static uint64_t heap_ptr;
static uint64_t heap_size;
static uint64_t prev_block;

SpinLock heap_spinlock;

uint64_t abs(long val) {
    return val < 0 ? val * -1 : val;
}

void mark_allocated(size_t position, size_t block_size) {
    long* block_start = (long*) position;
    block_start[0] = -1 * block_size;
    // Debug::printf("Block Footer Value: 0x%X\n", block_start[0]);
    block_start[block_size / 8 - 1] = -1 * block_size;
    // Debug::printf("Block Footer Value: %d\n", block_start[1]);
}

void mark_free(size_t position, size_t block_size) {
    long* block_start = (long*) position;
    block_start[0] = block_size;
    block_start[block_size / 8 - 1] = block_size;
    ((int*) position)[2] = prev_block;
    ((int*) position)[3] = 0;
    // Debug::printf("Previous: 0x%X, Next: 0x%X\n", ((int*) position)[2], ((int*) position)[3]);
    if (prev_block != 0) {
        int* previous_block_start = (int*) prev_block;
        previous_block_start[3] = position;
        // Debug::printf("Previous Block Next Value: 0x%X, Position Value: 0%X\n", previous_block_start[2], position);
    }
    prev_block = position;
    // Debug::printf("Block Footer Value: 0x%X\n", block_start[block_size / 8 - 1]);
}

void remove(size_t position) {
    int* free_block = (int*) position;
    uint64_t previous = free_block[2];
    uint64_t next = free_block[3];

    if (next == 0) {
        prev_block = previous;
    }
    else {
        int* next_block = (int*) next;
        next_block[2] = previous;
    }
    if (previous != 0) {
        int* previous_block = (int*) previous;
        previous_block[3] = next;
    }
}

void heap_init() {
    heap_ptr = (size_t) &_heap_start;
    heap_size = (size_t) &_heap_end - (size_t) &_heap_start;
    
    Debug::printf("Heap Start: 0x%X, Heap Size: 0x%X, Heap End: 0x%X\n", heap_ptr, heap_size, heap_end);

    mark_allocated(heap_ptr, 16);
    mark_free(heap_ptr + 16, heap_size - 32);
    mark_allocated(heap_ptr + heap_size - 16, 16);
}

void* malloc(size_t size, size_t alignment) {
    LockGuard<SpinLock> lg{heap_spinlock};
    
    size += 16;
    size = (size + alignment - 1) & ~(alignment - 1);
    void* tgt_block = 0;

    // Debug::printf("Malloc'ing %d bytes\n", size);

    uint64_t min_size = 0x7FFFFFFF;
    uint64_t chosen_block = 0;

    uint64_t current = prev_block;

    while (current != 0) {
        long* current_block = (long*) current;
        if (current_block[0] < 0) {
            // Debug::printf("Issue with block 0x%X, Should not be in free list.\n", current);
            return nullptr;
        }
        size_t current_block_size = current_block[0];
        if (current_block_size >= size) {
            if (current_block_size < min_size) {
                min_size = current_block_size;
                chosen_block = current;
            }
        }
        current = ((int*)current)[2];
    }

    if (chosen_block != 0) {
        remove(chosen_block);
        uint64_t leftover = min_size - size;
        if (leftover >= 8) {   
            // Debug::printf("")
            mark_allocated(chosen_block, size);
            mark_free(chosen_block + size, leftover);
        }
        else {
            mark_allocated(chosen_block, min_size);
        }
        tgt_block = (void*) (chosen_block + 8);
    }
    // Debug::printf("Given Block: 0x%X, Block Size: 0x%X\n", chosen_block + 8, size);
    return tgt_block;
}


// size_t align_up(size_t addr, size_t alignment) {
//     return (addr + alignment - 1) & ~(alignment - 1);
// }

// extern "C" void* malloc(size_t size, size_t alignment) {
//     // defaults to align to 4 bytes (word thingy from gheith though i am not sure)
//     size_t aligned_heap = align_up(current_heap, 4);

//     // Debug::printf("Block Size: 0x%X\n", size);

//     // check if out of memory
//     if (aligned_heap + size > heap_end) {
//       return 0;
//     }

//     void* allocated = (void*)aligned_heap;
//     current_heap = aligned_heap + size;
//     return allocated;
// }

extern "C" void free(void* ptr) {
    LockGuard<SpinLock> lg{heap_spinlock};

    if (ptr == 0) {
        Debug::printf("Freeing nullptr\n");
        return;
    }
    if (ptr < &_heap_start || ptr > &_heap_end) {
        Debug::printf("Freeing outside of heap: 0x%X\n", ptr);
        return;
    }
    
    long* block = ((long*) ptr) - 1;
    long block_size = block[0] * -1;

    // Debug::printf("Free Block Size: %d\n", block[0]);

    if (block_size < 0) {
        Debug::printf("Freeing free block 0x%X\n", block);
        return;
    }

    long* left_block = (block - (abs(block[-1]) / 8));
    long* right_block = (block + abs(block[block_size / 8]) / 8);
    // Debug::printf("Address of Block: 0x%X, Address of Left: 0x%X, Address of Right: 0x%X\n", block, left_block, right_block);

    long left_block_size = left_block[0];
    long right_block_size = right_block[0];

    uint64_t new_start_block = (uint64_t) block;
    uint64_t new_region_size = block_size;

    // Debug::printf("Left Block Size: %d\n", left_block_size);
    if (left_block_size > 0) {
        // Debug::printf("Left Block 0x%X is Free!\n", left_block);
        remove((uint64_t) left_block);
        new_start_block = (uint64_t) left_block;
        new_region_size += left_block_size;
    }

    // Debug::printf("Right Block Size: %d\n", left_block_size);
    if (right_block_size > 0) {
        // Debug::printf("Right Block 0x%X is Free!\n", right_block);
        remove((uint64_t) right_block);
        new_region_size += right_block_size;
    }
    block[0] = block_size;
    mark_free(new_start_block, new_region_size);
}


struct HeapTestStruct
{
  uint8_t test;
};

void run_heap_tests() {
    uint64_t* block = (uint64_t*) malloc(256);
    for (int i = 0; i < 32; i++) {
        block[i] = (uint64_t) malloc(16);
        // Debug::printf("New %d Block: 0x%X\n", i, block[i]);
    }
    for (int i = 1; i < 32; i *= 2) {
        // Debug::printf("First %d Blocks: 0x%X\n", i, block[i]);
        free((void*) block[i]);
    }
    for (int i = 0; i < 32; i++) {
        // Debug::printf("Other %d Blocks: 0x%X\n", i, block[i]);
        free ((void*) block[i]);
    }

    // Test 1: Basic allocation
    void* block1 = malloc(256, 8);
    if (block1 != 0) {
        Debug::printf("Test 1 Passed: Allocated 256 bytes.\n");
    } else {
        Debug::printf("Test 1 Failed: Allocation returned null.\n");
    }

    // Test 2: Large allocation within heap size
    char* block2 = (char*) malloc(0x10000000);
    if (block2 != 0) {
        Debug::printf("Test 2 Passed: Allocated 500000 bytes.\n");
    } else {
        Debug::printf("Test 2 Failed: Allocation returned null.\n");
    }
    // free(block2);
    // Test 3: Allocation exceeding available heap space
    void* block3 = malloc(0x20000000);  // This should fail
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
    // size_t remaining_space = heap_end - current_heap;
    // Debug::printf("0x%X\n", remaining_space);
    // void* block5 = malloc(remaining_space);
    // if (block5 != 0) {
    //     Debug::printf("Test 5 Passed: Allocated remaining heap space.\n");
    // } else {
    //     Debug::printf("Test 5 Failed: Allocation returned null.\n");
    // }
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

void operator delete(void* ptr, size_t sz) noexcept
{
  free(ptr);
}

void operator delete[](void* ptr, size_t sz) noexcept
{
  free(ptr);
}

// // Undefined Delete Reference Fix

extern "C" void __cxa_atexit() {}

extern "C" void __dso_handle() {}

extern "C" void __cxa_pure_virtual() {}