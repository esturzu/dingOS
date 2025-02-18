#include "heap.h"

#include "atomics.h"
#include "definitions.h"
#include "printf.h"
#include "stdint.h"

extern "C" char _end;
extern "C" char _heap_start;
extern "C" char _heap_end;

static size_t current_heap = (size_t)&_heap_start;
static const size_t heap_end = (size_t)&_heap_end;

static uint64_t heap_ptr;
static uint64_t heap_size;
static uint64_t prev_block;

SpinLock heap_spinlock;

void mark_allocated(size_t position, size_t block_size) {
  long* block_start = (long*)position;
  block_start[0] = -1 * block_size;
  block_start[block_size / 8 - 1] = -1 * block_size;
}

void mark_free(size_t position, size_t block_size) {
  long* block_start = (long*)position;
  block_start[0] = block_size;
  block_start[block_size / 8 - 1] = block_size;
  ((int*)position)[2] = prev_block;
  ((int*)position)[3] = 0;
  if (prev_block != 0) {
    int* previous_block_start = (int*)prev_block;
    previous_block_start[3] = position;
  }
  prev_block = position;
}

void remove(size_t position) {
  int* free_block = (int*)position;
  uint64_t previous = free_block[2];
  uint64_t next = free_block[3];

  if (next == 0) {
    prev_block = previous;
  } else {
    int* next_block = (int*)next;
    next_block[2] = previous;
  }
  if (previous != 0) {
    int* previous_block = (int*)previous;
    previous_block[3] = next;
  }
}

void heap_init() {
  heap_ptr = (size_t)&_heap_start;
  heap_size = (size_t)&_heap_end - (size_t)&_heap_start;

  dPrintf("Heap Start: 0x%X, Heap Size: 0x%X, Heap End: 0x%X\n", heap_ptr,
                heap_size, heap_end);

  mark_allocated(heap_ptr, 16);
  mark_free(heap_ptr + 16, heap_size - 32);
  mark_allocated(heap_ptr + heap_size - 16, 16);
}

void* malloc(size_t size, size_t alignment) {
  LockGuard<SpinLock> lg{heap_spinlock};

  size += 16;
  size = (size + alignment - 1) & ~(alignment - 1);
  void* tgt_block = 0;

  uint64_t min_size = 0x7FFFFFFF;
  uint64_t chosen_block = 0;

  uint64_t current = prev_block;

  while (current != 0) {
    long* current_block = (long*)current;
    if (current_block[0] < 0) {
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
      mark_allocated(chosen_block, size);
      mark_free(chosen_block + size, leftover);
    } else {
      mark_allocated(chosen_block, min_size);
    }
    tgt_block = (void*)(chosen_block + 8);
  }
  return tgt_block;
}

extern "C" void free(void* ptr) {
  LockGuard<SpinLock> lg{heap_spinlock};

  if (ptr == 0) {
    dPrintf("Freeing nullptr\n");
    return;
  }
  if (ptr < &_heap_start || ptr > &_heap_end) {
    dPrintf("Freeing outside of heap: 0x%X\n", ptr);
    return;
  }

  long* block = ((long*)ptr) - 1;
  long block_size = block[0] * -1;

  if (block_size < 0) {
    dPrintf("Freeing free block 0x%X\n", block);
    return;
  }

  long* left_block = (block - (ABS(block[-1]) / 8));
  long* right_block = (block + ABS(block[block_size / 8]) / 8);

  long left_block_size = left_block[0];
  long right_block_size = right_block[0];

  uint64_t new_start_block = (uint64_t)block;
  uint64_t new_region_size = block_size;

  if (left_block_size > 0) {
    remove((uint64_t)left_block);
    new_start_block = (uint64_t)left_block;
    new_region_size += left_block_size;
  }

  if (right_block_size > 0) {
    remove((uint64_t)right_block);
    new_region_size += right_block_size;
  }
  block[0] = block_size;
  mark_free(new_start_block, new_region_size);
}

// C++ Operators
// Citations
// https://en.cppreference.com/w/cpp/memory/new/operator_new
// https://en.cppreference.com/w/cpp/memory/new/operator_delete

void* operator new(size_t count) { return malloc(count, 8); }

void* operator new[](size_t count) { return malloc(count, 8); }

void* operator new(size_t count, align_val_t al) { return malloc(count, al); }

void* operator new[](size_t count, align_val_t al) { return malloc(count, al); }

void operator delete(void* ptr) noexcept { free(ptr); }

void operator delete[](void* ptr) noexcept { free(ptr); }

void operator delete(void* ptr, size_t sz) noexcept { free(ptr); }

void operator delete[](void* ptr, size_t sz) noexcept { free(ptr); }

// Undefined Delete Reference Fix

extern "C" void __cxa_atexit() {}

extern "C" void __dso_handle() {}

extern "C" void __cxa_pure_virtual() {}
