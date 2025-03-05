#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#include "sd.h"
#include "stdint.h"

// This wrapper provides a uniform interface for reading and writing blocks
// from the SD card using multi-block operations.
class BlockIO {
public:
    static constexpr uint32_t BLOCK_SIZE = SD::BLOCKSIZE;  // 512 bytes per block

    // Read multiple blocks from disk
    static void read(uint32_t start_block, uint32_t num_blocks, char* buffer) {
        SD::read(start_block, num_blocks, (uint8_t*)buffer);
    }

    // Write multiple blocks to disk
    static void write(uint32_t start_block, uint32_t num_blocks, const char* buffer) {
        SD::write(start_block, num_blocks, (uint8_t*)buffer);
    }
};

#endif  // BLOCK_IO_H
