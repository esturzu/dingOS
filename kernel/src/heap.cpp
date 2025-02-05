#include "stdint.h"
#include "heap.h"
#include "printf.h"

extern "C" char _end; 

static size_t current_heap = HEAP_START;

size_t align_up(size_t addr, size_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

extern "C" void* malloc(size_t size, size_t alignment) {
    // defaults to align to 4 bytes (word thingy from gheith though i am not sure)
    size_t aligned_heap = align_up(current_heap, alignment);

    // check if out of memory
    if (aligned_heap + size > HEAP_END) {
        return 0;
    }

    void* allocated = (void*)aligned_heap;
    current_heap = aligned_heap + size;
    return allocated;
}

extern "C" void free(void* pointer) {
    // haven't done it lol
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

// Undefined Delete Reference Fix

extern "C" void __cxa_atexit() {}

extern "C" void __dso_handle() {}

extern "C" void __cxa_pure_virtual() {}