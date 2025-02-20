#ifndef _BLOCK_IO_H_
#define _BLOCK_IO_H_

#include "stdint.h"
#include "printf.h"
#include "physmem.h"

//
// Base class for block devices (disks, files, etc.).
// This version stores blocks **in memory** for now.
//
// Assumptions:
// - Block size is a power of 2
// - Caller is responsible for bounds checking & locking
//
class BlockIO {
protected:
    char* storage;  // Memory buffer acting as "disk"
    uint32_t total_blocks;

public:
    const uint32_t block_size;

    BlockIO(uint32_t block_size, uint32_t num_blocks) 
        : block_size(block_size), total_blocks(num_blocks) {
        storage = (char*) PhysMem::allocate_frame();  // Allocate RAM storage
        if (!storage) debug_printf("BlockIO: Failed to allocate memory storage\n");
    }

    virtual ~BlockIO() {
        PhysMem::free_frame(storage);  // Release storage memory
    }

    // Get total size in bytes
    virtual uint32_t size_in_bytes() {
        return total_blocks * block_size;
    }

    // Get total number of blocks
    uint32_t size_in_blocks() {
        return total_blocks;
    }

    // Read a block and put its bytes in buffer
    virtual void read_block(uint32_t block_number, char* buffer) {
        if (block_number >= total_blocks) {
            debug_printf("BlockIO: read_block out of bounds\n");
        }
        char* source = storage + (block_number * block_size);
        for (uint32_t i = 0; i < block_size; i++) {
            buffer[i] = source[i];
        }
    }

    // Write a block from buffer into storage
    virtual void write_block(uint32_t block_number, const char* buffer) {
        if (block_number >= total_blocks) {
            debug_printf("BlockIO: write_block out of bounds\n");
        }
        char* dest = storage + (block_number * block_size);
        for (uint32_t i = 0; i < block_size; i++) {
            dest[i] = buffer[i];
        }
    }

    // Read a portion of a block
    virtual int64_t read(uint32_t offset, uint32_t n, char* buffer);

    // Read up to "n" bytes starting at "offset"
    virtual int64_t read_all(uint32_t offset, uint32_t n, char* buffer);
};

#endif  // _BLOCK_IO_H_
