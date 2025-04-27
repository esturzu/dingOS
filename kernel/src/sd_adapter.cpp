#include "sd_adapter.h"

 /**
  * Constructor initializes the adapter with a specific block size.
  * Calls the parent BlockIO constructor to set up the block size.
  * 
  * @param block_size Size of filesystem blocks in bytes
  */
 SDAdapter::SDAdapter(uint32_t block_size) : BlockIO(block_size) {}
 
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
  * Reads a complete filesystem block from the SD card.
  * 
  * The function converts the filesystem block number to SD card sector numbers
  * and reads the required sectors into the provided buffer.
  * 
  * @param block_number The filesystem block number to read
  * @param buffer Pointer to memory where the block data will be stored
  */
 void SDAdapter::read_block(uint32_t block_number, char* buffer) {
     // Calculate how many SD sectors make up one filesystem block
     // For example, if block_size=1024 and SD::BLOCKSIZE=512, then sd_block_count=2
     uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
     
     // Calculate the starting sector on the SD card
     // Maps the filesystem block to physical sectors
     uint32_t sd_start = block_number * sd_block_count;
     
     // Read the sectors from the SD card into the buffer
     // The SD::read function reads 'sd_block_count' sectors starting from sector 'sd_start'
     uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)buffer);
     
     // Check if read was successful by comparing bytes read with expected block size
     if (bytes != block_size) {
         printf("SDAdapter::read_block: Failed to read block %u\n", block_number);
     }
 }
 
 /**
  * Writes data to a block on the SD card.
  * 
  * For full block writes, this function directly writes the buffer to the SD card.
  * For partial block writes, it performs a read-modify-write operation to
  * ensure that unmodified portions of the block remain intact.
  * 
  * @param block_number The filesystem block number to write to
  * @param buffer Pointer to the data to be written
  * @param offset Offset within the block to start writing at
  * @param n Number of bytes to write
  */
 void SDAdapter::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
     printf("DEBUG: SDAdapter::write_block: block %u, offset %u, size %u\n", 
           block_number, offset, n);
     
     // Calculate SD card sector mapping
     // Convert filesystem blocks to SD card sectors
     uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
     uint32_t sd_start = block_number * sd_block_count;
     
     // Check if we need to perform a partial write
     // This happens when we're not writing a complete block or starting at non-zero offset
     if (offset > 0 || n < block_size) {
         printf("DEBUG: SDAdapter::write_block: Partial block write, doing read-modify-write\n");
         
         // Allocate a temporary buffer for the entire block
         uint8_t* temp_buffer = new uint8_t[block_size];
         
         // First read the current block contents
         // This is necessary to preserve data we're not modifying
         uint32_t read_bytes = SD::read(sd_start, sd_block_count, temp_buffer);
         
         if (read_bytes != block_size) {
             printf("DEBUG: SDAdapter::write_block: Read failed (%u/%u bytes)\n", 
                   read_bytes, block_size);
         }
         
         // Update only the portion that needs to change
         // Copy the new data byte-by-byte into the appropriate position in the block
         for (uint32_t i = 0; i < n; i++) {
             temp_buffer[offset + i] = buffer[i];
         }
         
         // Write the modified block back to the SD card
         uint32_t bytes = SD::write(sd_start, sd_block_count, temp_buffer);
         
         // Free the temporary buffer to avoid memory leaks
         delete[] temp_buffer;
         
         // Check if the write was successful
         if (bytes != block_size) {
             printf("DEBUG: SDAdapter::write_block: Write failed (%u/%u bytes)\n", 
                   bytes, block_size);
         } else {
             printf("DEBUG: SDAdapter::write_block: Write successful\n");
         }
     } else {
         // Simple case - writing an entire block
         // We can directly write the buffer without read-modify-write
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
     printf("DEBUG: SDAdapter::write: offset=%u, size=%u\n", offset, n);
 
     // Get the total size of the device
     auto sz = size_in_bytes();
     
     // Check if the offset is beyond the device size
     if (offset > sz) return -1;
     
     // Check if we're at the end of the device
     if (offset == sz) return 0;
 
     // Calculate the actual number of bytes we can write
     // This ensures we don't try to write beyond the device size
     auto actual_n = local_min(n, sz - offset);
     
     // Calculate which block contains this offset
     auto block_number = offset / block_size;
     
     // Calculate the position within that block
     auto offset_in_block = offset % block_size;
     
     // Calculate how many bytes we can write in this block
     // This is limited by either the block boundary or the requested write size
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
     printf("DEBUG: SDAdapter::write_all: offset=%u, size=%u\n", offset, n);
     
     int64_t total = 0;
     
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
     
     // Return the total number of bytes written
     return total;
 }