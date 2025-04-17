#ifndef _HEAP_H_
#define _HEAP_H_

#include "definitions.h"
#include "stdint.h"

extern "C" uint64_t* _end;
extern "C" uint64_t* _heap_start;
extern "C" uint64_t* _heap_end;

extern "C" void* malloc(size_t size, size_t alignment = 8);
extern "C" void free(void* pointer);
void heap_init();
void run_heap_tests();
#endif  // _HEAP_H_
