#ifndef _SD_ADAPTER_H_
#define _SD_ADAPTER_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"

/**
 * SDAdapter bridges the BlockIO abstraction with the SD card driver.
 * 
 * This class translates filesystem block operations into SD card sector operations.
 * It serves as the equivalent of the IDE class in the 439 code, except the IDE class
 * also contained the SD driver logic. This is purely an adapter between layers.
 * 
 * When filesystem operations are called, they'll use these implementations of the
 * BlockIO virtual class methods to perform actual I/O operations.
 */
 inline void mem_copy(void* dest, const void* src, uint32_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
}
 class SDAdapter : public BlockIO {
    public:
        /**
         * Constructor initializes the adapter with a specific block size.
         * @param block_size Size of filesystem blocks in bytes (typically 1024, 2048, or 4096)
         */
        SDAdapter(uint32_t block_size);
        
        /**
         * Virtual destructor for proper inheritance.
         * Flushes all dirty cache entries before destruction.
         */
        virtual ~SDAdapter();
        
        /**
         * Returns the size of the entire SD card in bytes.
         * @return A large value representing the capacity of the SD card
         */
        uint32_t size_in_bytes() override;
        
        /**
         * Reads a complete block from the SD card or cache.
         * Checks cache first before reading from SD card.
         * 
         * @param block_number The filesystem block number to read
         * @param buffer Pointer to the buffer where the block data will be stored
         */
        void read_block(uint32_t block_number, char* buffer) override;
        
        /**
         * Writes data to a block on the SD card or cache.
         * Uses different strategies for full vs. partial block writes.
         * 
         * @param block_number The filesystem block number to write to
         * @param buffer Pointer to the data to be written
         * @param offset Offset within the block to start writing at
         * @param n Number of bytes to write
         */
        void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override;
        
        /**
         * Writes data to the SD card starting at a specified byte offset.
         * Translates byte offsets to block numbers and block offsets.
         * 
         * @param offset Offset in bytes from the start of the SD card
         * @param n Number of bytes to write
         * @param buffer Pointer to the data to be written
         * @return Number of bytes actually written, or -1 on error
         */
        int64_t write(uint32_t offset, uint32_t n, char* buffer) override;
        
        /**
         * Writes all data to the SD card starting at a specified offset.
         * Makes multiple write calls if necessary to write all data.
         * 
         * @param offset Offset in bytes from the start of the SD card
         * @param n Number of bytes to write
         * @param buffer Pointer to the data to be written
         * @return Total number of bytes written, or negative value on error
         */
        int64_t write_all(uint32_t offset, uint32_t n, char* buffer) override;
    
        /**
         * Flushes all dirty blocks from cache to disk.
         * Should be called periodically or before unmounting the filesystem.
         */
        void flush_cache();
    
    private:
        /**
         * Cache entry structure for block caching
         */
        struct CacheEntry {
            uint32_t block_number;  // Block number (UINT32_MAX = invalid/empty)
            bool dirty;             // Whether block has been modified
            uint64_t last_access;   // For LRU replacement policy
            char* data;             // Block data
        };
        
        static const uint32_t CACHE_SIZE = 32;  // Size of the block cache
        CacheEntry cache[CACHE_SIZE];           // Cache entries
        uint64_t access_counter;                // For LRU tracking
        
        /**
         * Returns the minimum of two values, used for bound checking.
         */
        uint32_t local_min(uint32_t a, uint32_t b);
        
        /**
         * Finds a block in the cache.
         * 
         * @param block_number Block number to find
         * @return Cache index if found, -1 if not in cache
         */
        int find_cached_block(uint32_t block_number);
        
        /**
         * Updates or adds a block to the cache.
         * 
         * @param block_number Block number to cache
         * @param data Data buffer (nullptr = load from disk)
         * @param dirty Whether the block is dirty
         */
        void cache_block(uint32_t block_number, char* data, bool dirty);
        
        /**
         * Flushes a specific cached block to disk if dirty.
         * 
         * @param cache_index Index in the cache array
         */
        void flush_cached_block(int cache_index);
        
        /**
         * Finds the least recently used cache entry.
         * 
         * @return Index of the LRU cache entry
         */
        int find_lru_entry();
        
        /**
         * Performs a read-modify-write operation for true partial writes.
         * 
         * @param block_number Block to modify
         * @param buffer Source data
         * @param offset Offset within block
         * @param n Number of bytes to write
         */
        void partial_write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n);
    };

#endif // _SD_ADAPTER_H_