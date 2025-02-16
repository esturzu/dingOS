#ifndef _HEAP_H_
#define _HEAP_H_

#include "definitions.h"
#include "stdint.h"  // Or your equivalent file with size_t defined

extern "C" char _end;  // Linker script will define this symbol
extern "C" char _heap_start;

extern "C" void* malloc(size_t size, size_t alignment = 4);
extern "C" void free(void* pointer);
void heap_init();
void run_heap_tests();
#endif  // _HEAP_H_
