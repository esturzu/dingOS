#include "heap.h"

#include "atomics.h"
#include "definitions.h"
#include "printf.h"
#include "stdint.h"

// For any sort of reference, you can look up implict free list or use
// this PDF to understand: https://my.eng.utah.edu/~cs4400/malloc-2.pdf

// Linker Defined Symbols
extern "C" uint64_t* _end;
extern "C" uint64_t* _heap_start;
extern "C" uint64_t* _heap_end;

static uint64_t heap_size;

// Tracks the first available free block
static uint64_t free_head;

// Lock to prevent heap race conditions
SpinLock heap_spinlock;

// Marks a region as allocated by setting the first 8 bytes to the
// size of the region (negative to indicate allocated)
// Also marks the last 8 bytes to make it easier to get the
// "head" address of the region during coalescing
void mark_allocated(size_t position, size_t block_size) {
  long* block_start = (long*)position;
  block_start[0] = -1 * block_size;
  block_start[block_size / 8 - 1] = -1 * block_size;
}

// Marks a region as free by setting the first 8 bytes to the
// size of the region (positive to indicate freed)
// Also marks the last 8 bytes to make it easier to get the
// "head" address of the region during coalescing
void mark_free(size_t position, size_t block_size) {
  // debug_printf("freeing block: 0x%X\n", position);

  long* block_start = (long*)position;
  block_start[0] = block_size;
  block_start[block_size / 8 - 1] = block_size;

  // Let offset [2] = prev, offset [3] = next
  ((int*)position)[2] = 0;          // prev = 0 (we are new head)
  ((int*)position)[3] = free_head;  // next = old head

  // If we had a head, set head->prev = new block
  if (free_head != 0) {
    ((int*)free_head)[2] = position;  // oldHead->prev = position
  }

  // Now this block is the new head
  free_head = position;
}

// Used to remove an element from the free list. Typically done when a block is
// actually being freed in the 'free' method.
void remove(size_t position) {
  // Get the current block we're removing and the addresses of its neighbors
  int* free_block = (int*)position;
  uint64_t prev = free_block[2];
  uint64_t next = free_block[3];

  // If prev == 0, we are head
  if (prev == 0) {
    // Remove head
    free_head = next;
  } else {
    // prev->next = next
    ((int*)prev)[3] = next;
  }

  // If next != 0, next->prev = prev
  if (next != 0) {
    ((int*)next)[2] = prev;
  }
}

// Initialize the heap by checking locations to ensure proper setup
// Also sets guard regions and makes entire heap the start of the free list
void heap_init() {
  char* heap = (char*)&_heap_start;

  heap_size = (size_t)&_heap_end - (size_t)&_heap_start;

  for (int i = 0; i < heap_size; i++) {
    heap[i] = 0;
  }

  debug_printf("Heap Start: 0x%X, Heap Size: 0x%X, Heap End: 0x%X\n",
               (size_t)&_heap_start, heap_size, (size_t)&_heap_end);

  // Sets guard regions and free space
  mark_allocated((size_t)&_heap_start, 16);
  mark_free((size_t)&_heap_start + 16, heap_size - 32);
  mark_allocated((size_t)&_heap_start + heap_size - 16, 16);
}

// Malloc, used to allocate blocks of variable size for external use
void* malloc(size_t size, size_t alignment) {
  LockGuard<SpinLock> lg{heap_spinlock};

  // Adds 16 to size to account for last and first 8 bytes being used as
  // metadata
  size += 16;
  size = (size + alignment - 1) & ~(alignment - 1);
  void* tgt_block = 0;

  // Basic search for best fit block in list
  uint64_t min_size = 0x7FFFFFFF;
  uint64_t chosen_block = 0;

  uint64_t current = free_head;

  // Loops through the free list to try to identify the best fit block.
  // Maybe improve by limiting amount of searches if fragmentation becomes too
  // high
  while (current != 0) {
    long* current_block = (long*)current;
    // If the current block is already allocated, we have a problem
    if (current_block[0] < 0) {
      return nullptr;
    }
    size_t current_block_size = current_block[0];
    // Makes sure the block fits the requested size
    if (current_block_size >= size) {
      // Gets the best fit
      if (current_block_size < min_size) {
        min_size = current_block_size;
        chosen_block = current;
      }
    }
    // Go to the next block in the list.
    current = ((int*)current)[3];
  }

  // If there is a block that will fit
  if (chosen_block != 0) {
    remove(chosen_block);
    uint64_t leftover = min_size - size;
    // If leftover is signficant enough that another block should be made
    // Mark the leftover area as free and the exact fit as requested
    if (leftover > 16) {
      mark_allocated(chosen_block, size);
      mark_free(chosen_block + size, leftover);
    } else {
      mark_allocated(chosen_block, min_size);
      size = min_size;
    }
    // Set the return value AFTER the metadata
    tgt_block = (void*)(chosen_block + 8);
  }
  // debug_printf("tgt_block: 0x%X, block end: 0x%X\n", tgt_block, tgt_block +
  // size);
  return tgt_block;
}

// Method to free a given pointer
// Might be paging but should add permissions check!
// (Also need to add someone to make sure any pointer inside a free region
// cannot be double freed. Maybe some loop/search)
extern "C" void free(void* ptr) {
  LockGuard<SpinLock> lg{heap_spinlock};

  // Safety checks, makes sure the pointer is within the region of the heap
  if (ptr == 0) {
    debug_printf("Freeing nullptr\n");
    return;
  }
  if (ptr < &_heap_start || ptr > &_heap_end) {
    debug_printf("Freeing outside of heap: 0x%X\n", ptr);
    return;
  }

  // Get the actual start address of the block, pointer passed in is after
  // metadata, so - 1
  long* block = ((long*)ptr) - 1;
  long block_size = block[0] * -1;

  // Make sure block is actually allocated
  if (block_size <= 0) {
    debug_printf("Freeing free block 0x%X\n", block);
    return;
  }

  // Get the left block and the right block by using current block size
  // and metadata at the end of the block region
  long* left_block = (block - (ABS(block[-1]) / 8));
  long* right_block = (block + ABS(block[block_size / 8]) / 8);

  long left_block_size = left_block[0];
  long right_block_size = right_block[0];

  uint64_t new_start_block = (uint64_t)block;
  uint64_t new_region_size = block_size;

  // Now checking to see if blocks to the positional / immediate left and right
  // are free If they are, we will turn the whole region into a singular,
  // cohesive free block.
  if (left_block_size > 0) {
    // Remove the original left block if it exists since we're going to replace
    // it And set the values for our new region accordingly.
    remove((uint64_t)left_block);
    new_start_block = (uint64_t)left_block;
    new_region_size += left_block_size;
  }

  // Repeat for right side
  if (right_block_size > 0) {
    remove((uint64_t)right_block);
    new_region_size += right_block_size;
  }

  // Mark the pointer where the block was as free so any attempts to free it
  // again result in failure
  char* block_temp = (char*)new_start_block;
  for (int i = 0; i < new_region_size; i++) {
    block_temp[i] = 0;
  }
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
