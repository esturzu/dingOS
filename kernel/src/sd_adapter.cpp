#include "sd_adapter.h"

SDAdapter::SDAdapter(uint32_t block_size) : BlockIO(block_size) {}

uint32_t SDAdapter::local_min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

 // Returns a large enough size to represent the entire device
 uint32_t SDAdapter::size_in_bytes() {
    return 0xFFFFFFFF; // Large value representing the whole SD card
}

void SDAdapter::read_block(uint32_t block_number, char* buffer) {
    // Calculate how many SD sectors make up one filesystem block
    uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
    // Calculate the starting sector
    uint32_t sd_start = block_number * sd_block_count;
    // Read the sectors
    uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)buffer);
    // Check if read was successful
    if (bytes != block_size) {
        printf("SDAdapter::read_block: Failed to read block %u\n", block_number);
    }
}

void SDAdapter::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
    printf("DEBUG: SDAdapter::write_block: block %u, offset %u, size %u\n", 
          block_number, offset, n);
    
    // Calculate SD card sector
    uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
    uint32_t sd_start = block_number * sd_block_count;
    
    // If we need to modify part of a block, first read the entire block
    if (offset > 0 || n < block_size) {
        printf("DEBUG: SDAdapter::write_block: Partial block write, doing read-modify-write\n");
        uint8_t* temp_buffer = new uint8_t[block_size];
        uint32_t read_bytes = SD::read(sd_start, sd_block_count, temp_buffer);
        
        if (read_bytes != block_size) {
            printf("DEBUG: SDAdapter::write_block: Read failed (%u/%u bytes)\n", 
                  read_bytes, block_size);
        }
        
        // Update the portion that needs to change
        for (uint32_t i = 0; i < n; i++) {
            temp_buffer[offset + i] = buffer[i];
        }
        
        // Write the modified block back
        uint32_t bytes = SD::write(sd_start, sd_block_count, temp_buffer);
        delete[] temp_buffer;
        
        if (bytes != block_size) {
            printf("DEBUG: SDAdapter::write_block: Write failed (%u/%u bytes)\n", 
                  bytes, block_size);
        } else {
            printf("DEBUG: SDAdapter::write_block: Write successful\n");
        }
    } else {
        // Simple case - writing an entire block
        printf("DEBUG: SDAdapter::write_block: Full block write\n");
        uint32_t bytes = SD::write(sd_start, sd_block_count, (uint8_t*)buffer);
        if (bytes != block_size) {
            printf("DEBUG: SDAdapter::write_block: Write failed (%u/%u bytes)\n", 
                  bytes, block_size);
        } else {
            printf("DEBUG: SDAdapter::write_block: Write successful\n");
        }
    }
}

int64_t SDAdapter::write(uint32_t offset, uint32_t n, char* buffer) {
    printf("DEBUG: SDAdapter::write: offset=%u, size=%u\n", offset, n);

    auto sz = size_in_bytes();
    if (offset > sz) return -1;
    if (offset == sz) return 0;

    auto actual_n = local_min(n, sz - offset);
    auto block_number = offset / block_size;
    auto offset_in_block = offset % block_size;
    auto bytes_this_block = local_min(block_size - offset_in_block, actual_n);

    write_block(block_number, buffer, offset_in_block, bytes_this_block);
    return bytes_this_block;
}

int64_t SDAdapter::write_all(uint32_t offset, uint32_t n, char* buffer) {
    printf("DEBUG: SDAdapter::write_all: offset=%u, size=%u\n", offset, n);
    int64_t total = 0;
    while (n > 0) {
        int64_t written = write(offset, n, buffer);
        if (written <= 0) return (total > 0) ? total : written;
        offset += written;
        buffer += written;
        n -= written;
        total += written;
    }
    return total;
}
