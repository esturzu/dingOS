#ifndef _HEAP_H_
#define _HEAP_H_

#define HEAP_START_ADDR ((size_t) 0x10000)
#define HEAP_END_ADDR   ((size_t) 0x20000)

typedef struct {
    size_t lock;
    size_t offset;
} HeapMetadata;

#define ROUNDUP16(value)  (((value) + 15) / 16)
#define HEAP_METADATA_PTR ((HeapMetadata*) HEAP_START_ADDR)
#define HEAP_DATA_ADDR    ROUNDUP16((size_t) (HEAP_METADATA_PTR + 1))

extern void* malloc(size_t size);
extern void free(void* pointer);

#endif
