#include "stdint.h"
#include "heap.h"
#include "debug.h"

// extern "C" int* _heap_start; 
// #define HEAP_START ((uint64_t) _heap_start)
// #define HEAP_SIZE  0x100000  // 1 MB heap size
// #define HEAP_END   (HEAP_START + HEAP_SIZE)

// static int* current_heap;
// static const uint32_t heap_end = HEAP_START + HEAP_SIZE;
// static uint32_t free_index = 0;

// void heap_init() {
//     current_heap = (int*) HEAP_START;
//     int num_ints = (HEAP_SIZE / 4);
// }

// extern "C" void* malloc(size_t size) {
//     if ((uint64_t) current_heap + size > HEAP_END) {
//         return nullptr;
//     }
//     void* ptr = current_heap;
//     current_heap += size;
//     return ptr;
// }

// extern "C" void free(void* ptr) {

// }

// struct HeapTestStruct
// {
//   uint64_t test;
// };

// void run_heap_tests() {
//     // Test 1: Basic allocation
//     char* block1 = (char*) malloc(256);
//     if (block1 != 0) {
//         debug_print("Test 1 Passed: Allocated 256 bytes.\n");
//     } else {
//         debug_print("Test 1 Failed: Allocation returned null.\n");
//     }

//     // Test 2: Large allocation within heap size
//     char* block2 = (char*) malloc(500000);
//     for (int i = 0; i < (500000); i++) {
//         block2[i] = 0xFF;
//     }
//     if (block2 != 0) {
//         debug_print("Test 2 Passed: Allocated 500000 bytes.\n");
//     } else {
//         debug_print("Test 2 Failed: Allocation returned null.\n");
//     }

//     // Test 3: Allocation exceeding available heap space
//     void* block3 = malloc(700000);  // This should fail
//     if (block3 == 0) {
//         debug_print("Test 3 Passed: Out-of-memory condition handled.\n");
//     } else {
//         debug_print("Test 3 Failed: Allocation succeeded unexpectedly.\n");
//     }

//     // Test 4: Testing new keyword
//     HeapTestStruct* block4 = new HeapTestStruct();
//     if (block4 != 0) {
//         debug_print("Test 4 Passed: New keyword succeeded.\n");
//     } else {
//         debug_print("Test 4 Failed: New keywork failed.\n");
//     }

//     // Test 5: Testing allocating all remaining heap space
//     size_t remaining_space = heap_end - (uint64_t)current_heap;
//     void* block5 = malloc(remaining_space);
//     if (block5 != 0) {
//         debug_print("Test 5 Passed: Allocated remaining heap space.\n");
//     } else {
//         debug_print("Test 5 Failed: Allocation returned null.\n");
//     }
// }

// // C++ Operators
// // Citations
// // https://en.cppreference.com/w/cpp/memory/new/operator_new
// // https://en.cppreference.com/w/cpp/memory/new/operator_delete

// void* operator new(size_t count)
// {
//   return malloc(count);
// }

// void* operator new[](size_t count)
// {
//   return malloc(count);
// }

// void* operator new(size_t count, align_val_t al)
// {
//   return malloc(count);
// }

// void* operator new[](size_t count, align_val_t al)
// {
//   return malloc(count);
// }

// void operator delete(void* ptr) noexcept
// {
//   free(ptr);
// }

// void operator delete[](void* ptr) noexcept
// {
//  free(ptr);
// }