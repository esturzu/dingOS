#include "block_io.h"
#include "printf.h"

// random helper to do like gheith style code from his libk that we haven't organized yet

// Return the smaller of two numbers
template<typename T>
T min(T a, T b) {
    return (a < b) ? a : b;
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
        read_block(block_number, buffer);  // Read full block
    } else {
        char temp[block_size];  // Temporary buffer
        read_block(block_number, temp);
        for (uint32_t i = 0; i < actual_n; i++) {
            buffer[i] = temp[offset_in_block + i];
        }
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
