#ifndef HEAP_H
#define HEAP_H

#include "stdint.h"

void* operator new(size_t size);
void operator delete(void* ptr) noexcept;
void run_heap_tests();


#endif // HEAP_H