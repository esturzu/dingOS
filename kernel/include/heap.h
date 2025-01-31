#ifndef _HEAP_H_
#define _HEAP_H_

#include "stdint.h"  // Or your equivalent file with size_t defined

extern "C" char _end;  // Linker script will define this symbol

#define HEAP_START ((size_t)&_end)  
#define HEAP_SIZE  0x100000          // Example: 1 MB heap size
#define HEAP_END   (HEAP_START + HEAP_SIZE)

void* malloc(size_t size, size_t alignment = 4);
void free(void* pointer);
void run_heap_tests();
#endif  // _HEAP_H_
