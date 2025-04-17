#include "heap.h"

#include "atomics.h"
#include "definitions.h"
#include "printf.h"
#include "stdint.h"
#include "definitions.h"
#include "vmm.h"

// For any sort of reference, you can look up implict free list or use
// this PDF to understand: https://my.eng.utah.edu/~cs4400/malloc-2.pdf

// Linker Defined Symbols
extern "C" uint64_t* _end;
extern "C" uint64_t* _heap_start;
extern "C" uint64_t* _heap_end;

#define ALIGN_8(x) (((x) + 7) & ~7)
#define NEXT_IDX 2
#define PREV_IDX 1
#define GUARD_SZ 16
#define FREE_META_SIZE 32

static long* start;

static long* end;

static uint64_t heap_size;

// Tracks the first available free block
static long* head_of_list;

// Lock to prevent heap race conditions
SpinLock* heap_spinlock;

// Marks a region as allocated by setting the first 8 bytes to the
// size of the region (negative to indicate allocated)
// Also marks the last 8 bytes to make it easier to get the
// "head" address of the region during coalescing
void mark_allocated(long* position, size_t block_size) {
    position[0] = ((long) block_size) * -1;
    position[(block_size / 8) - 1] = ((long) block_size) * -1;
}

// Marks a region as free by setting the first 8 bytes to the
// size of the region (positive to indicate freed)
// Also marks the last 8 bytes to make it easier to get the
// "head" address of the region during coalescing
void mark_free(long* position, size_t block_size) {
    position[0] = ((long) block_size);
    position[(block_size / 8) - 1] = ((long) block_size);
}

bool check(long* block_head) {
    if (block_head < start || block_head > end) {
        return false;
    }
    return ABS(block_head[0]) == ABS(block_head[(ABS(block_head[0]) / 8) - 1]);
}

// Initialize the heap by checking locations to ensure proper setup
// Also sets guard regions and makes entire heap the start of the free list
void heap_init() {
    start = (long*) VMM::phys_to_kernel_ptr(&_heap_start);
    end = (long*) VMM::phys_to_kernel_ptr(&_heap_end);

    heap_size = ((uint64_t) end) - ((uint64_t) start);

    head_of_list = 0;

    heap_spinlock = nullptr;

    mark_free((start), heap_size);

    heap_spinlock = new SpinLock();
}

// Malloc, used to allocate blocks of variable size for external use
void* malloc(size_t size, size_t alignment) {
    __asm__ volatile("dsb sy" ::: "memory");
    if (heap_spinlock != nullptr) {
        heap_spinlock->lock();
    }

    size += 16;
    size = ALIGN_8(size);

    long* result = nullptr;

    long* current = start;
    uint64_t res_sz = 0;

    while (current < end) {
        long temp_sz = current[0];
        if (temp_sz > 0 && temp_sz >= size) {
            result = current;
            res_sz = (uint64_t) temp_sz;
            break;
        }
        current += (ABS(current[0]) / 8);
    }

    if (result != nullptr) {
        if (res_sz - size > 24) {
            mark_allocated(result, size);
            mark_free(result + (size / 8), res_sz - size);
        }
        else {
            mark_allocated(result, res_sz);
        }
    }
    else {
        if (heap_spinlock != nullptr) {
            heap_spinlock->unlock();
        }
        return nullptr;
    }

    if (heap_spinlock != nullptr) {
        heap_spinlock->unlock();
    }

    return (result + 1);
}

// Method to free a given pointer
// Might be paging but should add permissions check!
// (Also need to add someone to make sure any pointer inside a free region
// cannot be double freed. Maybe some loop/search)
extern "C" void free(void* ptr) {
    __asm__ volatile("dsb sy" ::: "memory");
    if (heap_spinlock != nullptr) {
        heap_spinlock->lock();
    }

    long* block = ((long*) ptr) - 1;

    if (block < start || block > end) {
        if (heap_spinlock != nullptr) {
            heap_spinlock->unlock();
        }
        return;
    }

    long blk_sz = block[0] * -1;

    if (blk_sz <= 0 || !check(block)) {
        if (heap_spinlock != nullptr) {
            heap_spinlock->unlock();
        }
        return;
    }

    long* free_start = block;
    size_t free_sz = blk_sz;

    long* left_footer = block - 1;
    if (left_footer > start) {
        long* left = left_footer - (ABS(left_footer[0]) / 8) + 1; 
        if (check(left) && left[0] > 0) {
            free_start = left;
            free_sz += (size_t) left[0];
        }
    }
    
    long* right = block + (blk_sz / 8);
    if (right < end) {
        if (check(right) && right[0] > 0) {
            free_sz += (size_t) right[0];
        }
    }

    mark_free(free_start, free_sz);
    if (heap_spinlock != nullptr) {
        heap_spinlock->unlock();
    }
}

// C++ Operators
// Citations
// https://en.cppreference.com/w/cpp/memory/new/operator_new
// https://en.cppreference.com/w/cpp/memory/new/operator_delete

void* operator new(size_t count) { return malloc(count, 8); }

void* operator new[](size_t count) { return malloc(count, 8); }

void* operator new(size_t count, align_val_t al) { return malloc(count, al); }

void* operator new[](size_t count, align_val_t al) { return malloc(count, al); }

void operator delete(void* ptr) noexcept {
  // debug_printf("Freeing address 0x%X\n", ptr);
  free(ptr);
}

void operator delete[](void* ptr) noexcept {
  // debug_printf("Freeing address 0x%X\n", ptr);
  free(ptr);
}

void operator delete(void* ptr, size_t sz) noexcept { free(ptr); }

void operator delete[](void* ptr, size_t sz) noexcept { free(ptr); }

// Undefined Delete Reference Fix

extern "C" void __cxa_atexit() {}

extern "C" void __dso_handle() {}

extern "C" void __cxa_pure_virtual() {}
