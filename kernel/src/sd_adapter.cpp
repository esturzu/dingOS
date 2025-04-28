#include "sd_adapter.h"

/**
 * Constructor initializes the adapter with a specific block size.
 * Sets up the block cache with empty entries.
 * 
 * @param block_size Size of filesystem blocks in bytes
 */
SDAdapter::SDAdapter(uint32_t block_size) : BlockIO(block_size), access_counter(0) {
    // Initialize cache entries
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        cache[i].block_number = 0xFFFFFFFF;  // Invalid block number
        cache[i].dirty = false;
        cache[i].last_access = 0;
        cache[i].data = new char[block_size];  // Allocate buffer for each cache entry
    }
    
    debug_printf("SDAdapter: Initialized with block size %u and cache size %u\n", 
           block_size, CACHE_SIZE);
}

/**
 * Destructor flushes any dirty cache entries and frees memory
 */
SDAdapter::~SDAdapter() {
    // Flush any dirty blocks before destroying
    flush_cache();
    
    // Free allocated memory
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        delete[] cache[i].data;
    }
    
    debug_printf("SDAdapter: Destroyed after flushing cache\n");
}

/**
 * Helper function to return the minimum of two values.
 */
uint32_t SDAdapter::local_min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

/**
 * Returns the size of the entire SD card in bytes.
 * This is a simplification - a real implementation would query the device.
 * 
 * @return A large value (4GB) representing the whole SD card capacity
 */
uint32_t SDAdapter::size_in_bytes() {
    return 0xFFFFFFFF; // 4GB - Large value representing the whole SD card
}

/**
 * Find a block in the cache
 * 
 * @param block_number Block number to find
 * @return Cache index if found, -1 if not in cache
 */
int SDAdapter::find_cached_block(uint32_t block_number) {
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].block_number == block_number) {
            return i;  // Found in cache
        }
    }
    return -1;  // Not in cache
}

/**
 * Find the least recently used cache entry
 * 
 * @return Index of the LRU cache entry
 */
int SDAdapter::find_lru_entry() {
    uint64_t min_access = 0xFFFFFFFFFFFFFFFF;
    int lru_index = 0;
    
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].last_access < min_access) {
            min_access = cache[i].last_access;
            lru_index = i;
        }
    }
    
    return lru_index;
}

/**
 * Flushes a specific cached block to disk if dirty
 * 
 * @param cache_index Index in the cache array
 */
void SDAdapter::flush_cached_block(int cache_index) {
    if (!cache[cache_index].dirty) {
        return;  // Not dirty, no need to write
    }
    
    uint32_t block_number = cache[cache_index].block_number;
    
    // Skip invalid blocks
    if (block_number == 0xFFFFFFFF) {
        return;
    }
    
    // Calculate SD card sector mapping
    uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
    uint32_t sd_start = block_number * sd_block_count;
    
    // Write to SD card
    debug_printf("Flushing dirty block %u to SD card\n", block_number);
    
    uint32_t bytes = SD::write(sd_start, sd_block_count, (uint8_t*)cache[cache_index].data);
    
    if (bytes != block_size) {
        debug_printf("Flush failed for block %u (%u/%u bytes written)\n", 
               block_number, bytes, block_size);
    }
    
    // Mark as clean
    cache[cache_index].dirty = false;
}

/**
 * Flushes all dirty blocks to disk
 */
void SDAdapter::flush_cache() {
    debug_printf("Flushing all dirty blocks in cache\n");
    
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].dirty) {
            flush_cached_block(i);
        }
    }
}

/**
 * Add or update a block in the cache
 * 
 * @param block_number Block number to cache
 * @param data Data buffer (nullptr = load from disk)
 * @param dirty Whether the block is dirty
 */
void SDAdapter::cache_block(uint32_t block_number, char* data, bool dirty) {
    int cache_index = find_cached_block(block_number);
    
    if (cache_index >= 0) {
        // Block already in cache, update it
        if (data) {
            // Copy new data
            mem_copy(cache[cache_index].data, data, block_size);
        }
        cache[cache_index].dirty |= dirty;  // Preserve dirty flag if already dirty
        cache[cache_index].last_access = access_counter++;
        
        debug_printf("Updated cached block %u (dirty=%d)\n", block_number, cache[cache_index].dirty);
        return;
    }
    
    // Not in cache, find a slot (LRU replacement)
    cache_index = find_lru_entry();
    
    // If current entry is dirty, flush it first
    if (cache[cache_index].dirty) {
        flush_cached_block(cache_index);
    }
    
    // Replace with new block
    debug_printf("Adding block %u to cache slot %d\n", block_number, cache_index);
    
    cache[cache_index].block_number = block_number;
    cache[cache_index].dirty = dirty;
    cache[cache_index].last_access = access_counter++;
    
    if (data) {
        // Copy provided data
        mem_copy(cache[cache_index].data, data, block_size);
    } else {
        // Load from disk
        uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
        uint32_t sd_start = block_number * sd_block_count;
        
        debug_printf("Loading block %u from disk\n", block_number);
        
        uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)cache[cache_index].data);
        
        if (bytes != block_size) {
            debug_printf("Read failed for block %u (%u/%u bytes read)\n", 
                   block_number, bytes, block_size);
        }
    }
}

/**
 * Reads a complete filesystem block from the SD card or cache.
 * 
 * @param block_number The filesystem block number to read
 * @param buffer Pointer to memory where the block data will be stored
 */
void SDAdapter::read_block(uint32_t block_number, char* buffer) {
    debug_printf("Reading block %u\n", block_number);
    
    int cache_index = find_cached_block(block_number);
    
    if (cache_index >= 0) {
        // Block is in cache, just copy it to the buffer
        mem_copy(buffer, cache[cache_index].data, block_size);
        cache[cache_index].last_access = access_counter++;
        
        debug_printf("Block %u read from cache\n", block_number);
        return;
    }
    
    // Block not in cache, check if we should cache it
    // Currently caching all read blocks - could implement more sophisticated policy
    cache_block(block_number, nullptr, false);  // Will load from disk
    
    // Now it's in cache, find it and copy to buffer
    cache_index = find_cached_block(block_number);
    mem_copy(buffer, cache[cache_index].data, block_size);
}

/**
 * Helper for true partial block writes (needs read-modify-write)
 */
void SDAdapter::partial_write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
    debug_printf("Partial write to block %u (offset=%u, size=%u)\n", block_number, offset, n);
    
    int cache_index = find_cached_block(block_number);
    
    if (cache_index < 0) {
        // Not in cache, load it first
        cache_block(block_number, nullptr, false);
        cache_index = find_cached_block(block_number);
    }
    
    // Update the cached data
    mem_copy(cache[cache_index].data + offset, buffer, n);
    cache[cache_index].dirty = true;
    cache[cache_index].last_access = access_counter++;
    
    debug_printf("Block %u updated in cache\n", block_number);
}

/**
 * Writes data to a block on the SD card.
 * 
 * This optimized version uses the cache for both full and partial block writes,
 * performing read-modify-write only when necessary.
 * 
 * @param block_number The filesystem block number to write to
 * @param buffer Pointer to the data to be written
 * @param offset Offset within the block to start writing at
 * @param n Number of bytes to write
 */
void SDAdapter::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
    debug_printf("Write request for block %u (offset=%u, size=%u)\n", block_number, offset, n);
    
    // Case 1: Full block write - simplest case
    if (offset == 0 && n == block_size) {
        // Put directly in cache as dirty
        cache_block(block_number, buffer, true);
        return;
    }
    
    // Case 2: Partial block write - need to do read-modify-write
    partial_write_block(block_number, buffer, offset, n);
}

/**
 * Writes data to the SD card starting at a specified byte offset.
 * 
 * This function calculates which block and offset within that block
 * correspond to the given byte offset, and writes as many bytes as
 * possible within that block.
 * 
 * @param offset Byte offset from the start of the SD card
 * @param n Number of bytes to write
 * @param buffer Pointer to the data to be written
 * @return Number of bytes written, or -1 on error
 */
int64_t SDAdapter::write(uint32_t offset, uint32_t n, char* buffer) {
    debug_printf("Write at offset=%u, size=%u\n", offset, n);
    
    // Get the total size of the device
    auto sz = size_in_bytes();
    
    // Check if the offset is beyond the device size
    if (offset > sz) return -1;
    
    // Check if we're at the end of the device
    if (offset == sz) return 0;
    
    // Calculate the actual number of bytes we can write
    auto actual_n = local_min(n, sz - offset);
    
    // Calculate which block contains this offset
    auto block_number = offset / block_size;
    
    // Calculate the position within that block
    auto offset_in_block = offset % block_size;
    
    // Calculate how many bytes we can write in this block
    auto bytes_this_block = local_min(block_size - offset_in_block, actual_n);
    
    // Write to the block at the calculated offset
    write_block(block_number, buffer, offset_in_block, bytes_this_block);
    
    // Return the number of bytes we actually wrote
    return bytes_this_block;
}

/**
 * Writes all data to the SD card starting at a specified offset.
 * 
 * This function makes multiple calls to write() if necessary to
 * ensure all data is written, handling cases where the data spans
 * multiple blocks.
 * 
 * @param offset Byte offset from the start of the SD card
 * @param n Number of bytes to write
 * @param buffer Pointer to the data to be written
 * @return Total number of bytes written, or negative value on error
 */
int64_t SDAdapter::write_all(uint32_t offset, uint32_t n, char* buffer) {
    debug_printf("Write_all at offset=%u, size=%u\n", offset, n);
    
    int64_t total = 0;
    
    // Consider auto-flushing if writing large amounts of data
    bool should_auto_flush = (n > block_size * CACHE_SIZE / 2);
    
    // Keep writing until all data is written or an error occurs
    while (n > 0) {
        // Write as much as we can at the current offset
        int64_t written = write(offset, n, buffer);
        
        // Check for error or end of device
        if (written <= 0) {
            // If we've already written some data, return the total
            // Otherwise, return the error code
            return (total > 0) ? total : written;
        }
        
        // Update offset, buffer pointer, and remaining bytes for next iteration
        offset += written;
        buffer += written;
        n -= written;
        total += written;
    }
    
    // For large writes, consider auto-flushing to avoid keeping too much in memory
    if (should_auto_flush) {
        debug_printf("Auto-flushing cache after large write (%lld bytes)\n", total);
        flush_cache();
    }
    
    // Return the total number of bytes written
    return total;
}