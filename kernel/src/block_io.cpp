#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "stdint.h"
#include "sd.h"

bool streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    if (*a == *b) {
        printf("in streq and it's true");
    }
    return *a == *b;
}

void zero_memory(uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
}

void cpy(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Implements reading a full block (block_size bytes) using SD reads
void BlockIO::read_block(uint32_t block_number, char* buffer) {
    uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
    uint32_t sd_start = block_number * sd_block_count;
    uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)buffer);
    if (bytes != block_size) {
        printf("read_block: SD read failed (%u/%u bytes)\n", bytes, block_size);
    }
}

// Implements writing a full block (block_size bytes) using SD writes
void BlockIO::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
    uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
    uint32_t sd_start = block_number * sd_block_count;
    uint32_t bytes = SD::write(sd_start, sd_block_count, (uint8_t*)buffer);
    if (bytes != block_size) {
        printf("write_block: SD write failed (%u/%u bytes)\n", bytes, block_size);
    }
}


int64_t BlockIO::read(uint32_t offset, uint32_t desired_n, char* buffer) {
    auto sz = size_in_bytes();
    if (offset > sz) return -1;
    if (offset == sz) return 0;

    auto n = min(desired_n, sz - offset);
    auto block_number = offset / block_size;
    auto offset_in_block = offset % block_size;
    auto actual_n = min(block_size - offset_in_block, n);

    if (actual_n == block_size) {
        read_block(block_number, buffer);
    } else {
        char* temp = new char[block_size];
        zero_memory((uint8_t*)temp, block_size);
        read_block(block_number, temp);
        for (uint32_t i = 0; i < actual_n; i++) {
            buffer[i] = temp[offset_in_block + i];
        }
        delete[] temp;
    }
    return actual_n;
}

int64_t BlockIO::write(uint32_t offset, uint32_t desired_n, char* buffer) {
    auto sz = desired_n;
    auto n = min(desired_n, sz - offset);
    auto block_number = offset / block_size;
    auto offset_in_block = offset % block_size;
    auto actual_n = min(block_size - offset_in_block, n);

    if (actual_n == block_size) {
        write_block(block_number, buffer, offset, desired_n);
    } else {
        write_block(block_number, buffer, offset, desired_n);
    }
    return actual_n;
}

int64_t BlockIO::read_all(uint32_t offset, uint32_t n, char* buffer) {
    int64_t total_count = 0;
    while (n > 0) {
        int64_t cnt = read(offset, n, buffer);
        if (cnt < 0) return cnt;
        if (cnt == 0) return total_count;
        total_count += cnt;
        offset += cnt;
        n -= cnt;
        buffer += cnt;
    }
    return total_count;
}

int64_t BlockIO::write_all(uint32_t offset, uint32_t n, char* buffer) {
    int64_t total_count = 0;
    while (n > 0) {
        int64_t cnt = write(offset, n, buffer);
        if (cnt < 0) return cnt;
        if (cnt == 0) return total_count;
        total_count += cnt;
        offset += cnt;
        n -= cnt;
        buffer += cnt;
    }
    return total_count;
}
