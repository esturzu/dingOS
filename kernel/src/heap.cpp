

    

#include "stdint.h"
#include "heap.h"
#include "debug.h"

extern "C" {
    extern char _heap_start;
    extern char _heap_end;
}

static char* heap_ptr;
static char* heap_end;
static uint64_t heap_size;
static char* prev_block = 0;

int retrieve_num(int* position) {
    return position[0];
}

void insert_num(int* position, int val) {
    position[0] = val;
}

void mark_allocated(char* position, int num_bytes) {
    insert_num((int*) position, -1 * num_bytes);
    insert_num((int*) (position + num_bytes - 4), -1 * num_bytes);
}

void mark_free(char* position, int num_bytes) {
    insert_num((int*) position, num_bytes);
    insert_num((int*) (position + num_bytes - 4), num_bytes);
    insert_num((int*) (position + 4), (uint64_t) prev_block);
    insert_num((int*) (position + 8), 0);
    if (prev_block != 0) {
        insert_num((int*) (prev_block + 8), (uint64_t) (position));
    }
    prev_block = position;
}

void remove(char* block) {
    char* previous = (char*) retrieve_num((int*) (block + 4));
    char* next = (char*) retrieve_num((int*) (block + 8));

    if (next == 0) {
        prev_block = previous;
    }
    else {
        insert_num((int*) (next + 4), (uint64_t) previous);
    }
    if (previous != 0) {
        insert_num((int*) (previous + 8), (uint64_t) next);
    }
}

void heap_init() {
    heap_size = &_heap_end - &_heap_start;
    heap_ptr = (char*) &_heap_start;
    heap_end = (char*) &_heap_end; 
    for (int i = 0; i < heap_size; i++) {
        heap_ptr[i] = 0x0;
    }

    mark_allocated(heap_ptr, 8);
    mark_free((char*) (heap_ptr + 8), (&_heap_end - &_heap_start) - 16);
    mark_allocated((char*)(heap_end - 8), 8);
}

void* malloc(size_t size) {
    // // Align to 8 bytes
    size = ((size + 8) + 7) & ~7;

    void* block = 0;
    int min_block_size = 0x7FFFFFFF;

    char* cur = (char*) prev_block;

    while (cur != 0) {
        int block_size = retrieve_num((int*) cur);
        if (block_size < 0) {
            return nullptr;
        }

        if (block_size >= size) {
            if (block_size <= min_block_size) {
                min_block_size = block_size;
                block = (void*) cur;
            }
        }
        cur = (char*) retrieve_num((int*) (cur + 4));
    }
    
    if (block != 0) {
        remove((char*) block);
        int difference = min_block_size - size;
        if (difference >= 8) { 
            mark_allocated((char*)block, size);
            mark_free(((char*)block) + size, difference);
        }
        else {
            mark_allocated((char*)block, min_block_size);
        }
        block = ((char*) block) + 4;
    }
}

struct HeapTestStruct
{
  uint64_t test;
};

void run_heap_tests() {
    // Test 1: Basic allocation
    char* block1 = (char*) malloc(256);
    
    // for (int i = 0; i < 20; i++) {
        
    //     if (block1 == &heap_ptr[i]) {
    //         debug_print("ITS ME  ME ME ME \n");
    //     }
    //     else {
    //         debug_print("not me\n");
    //     }
    // }
    if (block1 != 0) {
        debug_print("Test 1 Passed: Allocated 256 bytes.\n");
    } else {
        debug_print("Test 1 Failed: Allocation returned null.\n");
    }

    // Test 2: Large allocation within heap size
    char* block2 = (char*) malloc(0x3D0);
    for (int i = 0; i < 0x3D0; i++) {
         block2[i] = 0xFF;
    }
    if (block2 != 0) {
        debug_print("Test 2 Passed: Allocated 500000 bytes.\n");
    } else {
        debug_print("Test 2 Failed: Allocation returned null.\n");
    }

    // // Test 3: Allocation exceeding available heap space
    // void* block3 = malloc(0xFFFFFFFF);  // This should fail
    // if (block3 == 0) {
    //     debug_print("Test 3 Passed: Out-of-memory condition handled.\n");
    // } else {
    //     debug_print("Test 3 Failed: Allocation succeeded unexpectedly.\n");
    // }

    // // Test 4: Testing new keyword
    // HeapTestStruct* block4 = new HeapTestStruct();
    // if (block4 != 0) {
    //     debug_print("Test 4 Passed: New keyword succeeded.\n");
    // } else {
    //     debug_print("Test 4 Failed: New keywork failed.\n");
    // }

    // // Test 5: Testing allocating all remaining heap space
    // size_t remaining_space = _heap_end - (uint64_t)heap_ptr;
    // void* block5 = (void*) 1; 
    // if (block5 != 0) {
    //     debug_print("Test 5 Passed: Allocated remaining heap space.\n");
    // } else {
    //     debug_print("Test 5 Failed: Allocation returned null.\n");
    // }
}

void* operator new(size_t size) {
    void* p =  malloc(size);
    return p;
}

void operator delete(void* p) noexcept {
}

void operator delete(void* p, size_t sz) {
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete[](void* p) noexcept {
}

void operator delete[](void* p, size_t sz) {
}