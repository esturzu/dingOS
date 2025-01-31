#include "heap.h"

void _lock_heap() {
    // TODO implement once locks become availble
}

void _unlock_heap() {
    // TODO implement once locks become availble
}

void* malloc(size_t size) {
    if (size == 0 || size >= HEAP_END_ADDR - HEAP_DATA_ADDR) return NULL;
    size_t size16 = (size + 15) / 16;
    size_t* offset16Ptr = &(HEAP_METADATA_PTR->offset);

    _lock_heap();
    size_t offset16 = *offset16Ptr;
    *offset16Ptr += size16;
    _unlock_heap();

    size_t address = HEAP_DATA_ADDR + offset16 * 16;
    if (address + size16 * 16 >= HEAP_END_ADDR) return NULL;
    return (void*) address;
}

void free(void* pointer) {}

