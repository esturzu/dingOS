/**
 * @file ext2.cpp
 * @brief Implementation of the ext2 filesystem operations.
 * 
 * This file implements the functions declared in ext2.h, providing
 * a working implementation of the ext2 filesystem. It includes operations
 * for traversing directories, reading and writing files, and managing
 * filesystem metadata.
 */

 #include "ext2.h"

 /**
  * GLOBAL VARIABLES
  * 
  * These variables are shared across the filesystem implementation
  * to provide access to key data structures and state information.
  */
 super_block* supa = nullptr;    // Pointer to the filesystem superblock
 BGDT* bgdt = nullptr;           // Pointer to the first block group descriptor
 uint32_t SB_offset = 1024;      // Superblock offset (always 1024 bytes from start of volume)
 uint32_t BGDT_index = 0;        // Block index of the Block Group Descriptor Table
 
 // Dirty flags to track which structures need to be written back to disk
 bool inode_bitmap_dirty = false;  // True when inode bitmap has been modified
 bool block_bitmap_dirty = false;  // True when block bitmap has been modified
 bool bgdt_dirty = false;          // True when BGDT has been modified
 bool dir_inode_dirty = false;     // True when a directory inode has been modified
 
 // In-memory caches of the bitmaps (allocated in Ext2 constructor)
 uint8_t* cached_inode_bitmap = nullptr;  // Bitmap of allocated/free inodes
 uint8_t* cached_block_bitmap = nullptr;  // Bitmap of allocated/free blocks
 
 
 /**
  * UTILITY FUNCTIONS
  */
 
 /**
  * Dump a block's contents for debugging.
  * 
  * Reads and displays blocks in a hexadecimal format, useful for
  * debugging filesystem structures directly from disk.
  * 
  * @param adapter Pointer to the SD adapter for disk access
  * @param block_start First block number to dump
  * @param count Number of consecutive blocks to dump
  */
 void dump_blocks(SDAdapter* adapter, uint32_t block_start, uint32_t count) {
     // Get the block size from the adapter
     const uint32_t block_size = adapter->block_size;
     
     // Allocate a temporary buffer to hold each block
     char* buffer = new char[block_size];
 
     // Loop through each requested block
     for (uint32_t i = 0; i < count; i++) {
         uint32_t block_num = block_start + i;
         
         // Read the block from disk
         adapter->read_block(block_num, buffer);
 
         // Print block header
         debug_printf("Block %u (size: %u bytes):\n", block_num, block_size);
         
         // Print block contents in 64-byte rows
         for (uint32_t offset = 0; offset < block_size; offset += 64) {
             // Print offset at start of each row
             debug_printf("  %04x: ", offset);
             
             // Print up to 64 bytes in hexadecimal format
             for (uint32_t j = 0; j < 64 && (offset + j) < block_size; j++) {
                 debug_printf("%02x ", (uint8_t)buffer[offset + j]);
             }
             debug_printf("\n");
         }
         debug_printf("\n");
     }
 
     // Free the temporary buffer
     delete[] buffer;
 }
 
 /**
  * Compare two strings for equality.
  */
 bool streq_ext(const char* a, const char* b) {
     // Compare characters until one string ends or characters differ
     while (*a && *b) {
         if (*a != *b) return false;
         a++; b++;
     }
     
     // Strings are equal if both reached their end simultaneously
     return *a == *b;  // Both strings end at the same position
 }
 
 /**
  * Calculate string length.
  */
 uint32_t strlen_ext(const char* str) {
     uint32_t len = 0;
     while (str[len]) len++;  // Count until null terminator
     return len;
 }
 
 /**
  * basically memset
  */
 void zero_memory(void* buffer, uint32_t size) {
     char* p = (char*)buffer;
     // Set each byte to zero individually
     for (uint32_t i = 0; i < size; i++) {
         p[i] = 0;
     }
 }
 
 /**
  * NODE METHODS IMPLEMENTATION
  */
 
 /**
  * Write a block to this node.
  * 
  * Handles writing data to a specific block within a file or directory.
  * If the block doesn't exist yet, it will be allocated.
  * 
  * @param block_number Logical block number within the file
  * @param buffer Data to write
  * @param offset Offset within the block to start writing
  * @param n Number of bytes to write
  */
 void Node::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
     debug_printf("DEBUG: Node::write_block: inode %u, block %u, offset %u, size %u\n", 
            number, block_number, offset, n);
     
     // This implementation only supports direct blocks (0-11)
     if (block_number >= 12) {
         debug_printf("DEBUG: Node::write_block: Only direct blocks (0-11) supported in this version\n");
         return;
     }
     
     // Check if the block is already allocated
     if (node->directLinked[block_number] == 0) {
         debug_printf("DEBUG: Node::write_block: Block not allocated, allocating new block\n");
         
         // Need to allocate a new block
         uint32_t new_block = allocate_block();
         if (new_block == 0) {
             debug_printf("DEBUG: Node::write_block: Failed to allocate new block\n");
             return;
         }
         
         debug_printf("DEBUG: Node::write_block: Allocated block %u\n", new_block);
         
         // Store the new block number in the inode's direct block array
         node->directLinked[block_number] = new_block;
     }
     
     debug_printf("DEBUG: Node::write_block: Writing to physical block %u\n", 
            node->directLinked[block_number]);
     
     // Write the data to the block
     sd_adapter->write_block(node->directLinked[block_number], buffer, offset, n);
     
     // Update file size if needed
     // Calculate the position of the last byte written:
     // (block_number * block_size) = start of block
     // + offset = position within block
     // + n = bytes written
     uint32_t end_pos = (block_number * block_size) + offset + n;
     debug_printf("DEBUG: Node::write_block: Current size=%u, new end_pos=%u\n", 
            node->size_of_iNode, end_pos);
     
     // If the write extends beyond the current file size, update the size
     if (end_pos > node->size_of_iNode) {
         debug_printf("DEBUG: Node::write_block: Updating inode size from %u to %u\n", 
                node->size_of_iNode, end_pos);
         node->size_of_iNode = end_pos;
         
         // Write the updated inode back to disk
         update_inode_on_disk();
     }
 }
 
 /**
  * Update inode on disk after changes.
  * 
  * Writes the in-memory inode data back to disk when it has been modified.
  */
 void Node::update_inode_on_disk() {
     debug_printf("DEBUG: Node::update_inode_on_disk: Saving inode %u (size=%u, type=0x%x)\n", 
            number, node->size_of_iNode, node->types_plus_perm);
     
     // Calculate the byte offset to the inode:
     // 1. Start with the block containing the inode table (bgdt->startingBlockAddress)
     // 2. Multiply by block size to get byte offset to start of table
     // 3. Add (index * iNode_size) to get offset to specific inode
     uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                            index * supa->iNode_size;
     
     debug_printf("DEBUG: Node::update_inode_on_disk: Writing to offset 0x%x, size=%u\n", 
            inode_offset, sizeof(iNode));
            
     // Write the inode data back to disk
     int64_t result = sd_adapter->write_all(inode_offset, sizeof(iNode), (char*)node);
     debug_printf("DEBUG: Node::update_inode_on_disk: Write result=%lld\n", result);
 }
 
 /**
  * Find and allocate a free block.
  * 
  * Searches the block bitmap for an unallocated block, marks it as allocated,
  * and returns its block number.
  * 
  * @return Allocated block number, or 0 if no free blocks are available
  */
 uint32_t Node::allocate_block() {
     debug_printf("DEBUG: Node::allocate_block: Finding free block for inode %u\n", number);
     
     // Detailed bitmap tracking
     uint32_t total_blocks = supa->num_Blocks;
     uint32_t free_blocks_in_bitmap = 0;
     uint32_t first_free_block = 0;
     
     // First scan: Count free blocks and find the first free block
     for (uint32_t i = 0; i < total_blocks; i++) {
         // Calculate the byte and bit position within the bitmap
         // byte_idx = i / 8: Each byte holds 8 bits
         // bit_idx = i % 8: Position within the byte (0-7)
         uint32_t byte_idx = i / 8;
         uint32_t bit_idx = i % 8;
         
         // Check if this bit is clear (0 = free, 1 = allocated)
         // (cached_block_bitmap[byte_idx] & (1 << bit_idx)) gets the bit value
         // If the result is 0, the block is free
         if ((cached_block_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
             free_blocks_in_bitmap++;
             if (first_free_block == 0) {
                 // Add 1 because block numbers are 1-based in ext2
                 first_free_block = i + 1;
             }
         }
     }
     
     // Check if we found any free blocks
     if (first_free_block == 0) {
         debug_printf("ERROR: No free blocks available!\n");
         return 0;
     }
     
     // Second scan: Find first free block (could be optimized by using first_free_block)
     for (uint32_t i = 0; i < total_blocks; i++) {
         uint32_t byte_idx = i / 8;
         uint32_t bit_idx = i % 8;
         
         if ((cached_block_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
             // Mark the block as used in the bitmap
             // (1 << bit_idx) creates a mask with only the bit we want to set
             // |= performs a bitwise OR to set this bit to 1
             cached_block_bitmap[byte_idx] |= (1 << bit_idx);
             
             debug_printf("DEBUG: Found free block %u (byte %u, bit %u)\n", 
                    i + 1, byte_idx, bit_idx);
                    
             // Mark the bitmap as dirty so it will be written back to disk
             block_bitmap_dirty = true;
             bgdt_dirty = true;
             
             // Return the block number (1-based in ext2)
             return i + 1;
         }
     }
     
     debug_printf("ERROR: No free blocks found despite initial count\n");
     return 0; // No free blocks
 }
 
 /**
  * DIRECTORY OPERATIONS
  */
 
 /**
  * List all entries in a directory.
  * 
  * Reads and displays information about all files and subdirectories
  * within the specified directory.
  * 
  * @param dir Node representing the directory to list
  */
 void list_directory(Node* dir) {
     // Verify this is actually a directory
     if (!dir->is_dir()) {
         debug_printf("Error: Not a directory\n");
         return;
     }
     
     debug_printf("Directory listing (inode %u):\n", dir->number);
     
     // Allocate a buffer for reading directory blocks
     // Size is determined by the filesystem's block size
     char* blockDir = new char[1024 << supa->block_size];
     if (!blockDir) {
         debug_printf("Error: Failed to allocate memory for directory block\n");
         return;
     }
     
     uint32_t bytes_so_far = 0;  // Track how many bytes we've processed
     uint32_t block_num = 0;     // Start with the first block
     
     // Process each block in the directory until we've read all the data
     while (bytes_so_far < dir->node->size_of_iNode) {
         // Read one block of directory entries
         dir->read_block(block_num, blockDir);
         
         uint32_t offset = 0;  // Offset within this block
         
         // Process entries within this block
         while (offset < (1024 << supa->block_size)) {
             // Cast the current position to a directory entry structure
             dir_entry* entry = (dir_entry*)(blockDir + offset);
             
             // Validate the entry size for corruption detection
             if (entry->size_entry == 0 || entry->size_entry > (1024 << supa->block_size) - offset) {
                 debug_printf("Corrupt directory entry at offset %u: size_entry=%u\n", offset, entry->size_entry);
                 break;
             }
             
             // Check if we've reached the end of valid entries
             if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                 break;
             }
             
             // Extract the name from the entry
             char entry_name[256];
             uint32_t name_len = entry->name_length;
             if (name_len > 255) name_len = 255;  // Safety check
             
             // Copy the name characters (name starts after the header fields)
             // Header is 8 bytes (iNodeNum=4, size_entry=2, name_length=1, type_indicator=1)
             for (uint32_t i = 0; i < name_len; i++) {
                 entry_name[i] = blockDir[offset + 8 + i];
             }
             entry_name[name_len] = '\0';  // Null-terminate the string
             
             // Determine the type of file
             char type_char = '?';
             Node* entry_node = new Node(1024 << supa->block_size, entry->iNodeNum, dir->sd_adapter);
             
             if (entry_node->is_dir()) {
                 type_char = 'D';  // Directory
             } else if (entry_node->is_file()) {
                 type_char = 'F';  // File
             } else if (entry_node->is_symlink()) {
                 type_char = 'L';  // Symbolic link
             }
             
             // Print the entry info (inode number, size, type, name)
             debug_printf("  %8u  %8u  %c  %s\n", 
                    entry->iNodeNum, 
                    entry_node->node->size_of_iNode, 
                    type_char, 
                    entry_name);
             
             delete entry_node;  // Clean up
             
             // Move to the next entry
             bytes_so_far += entry->size_entry;
             offset += entry->size_entry;
             
             // Check if we've reached the end of the block
             if (offset >= (1024 << supa->block_size)) {
                 break;
             }
         }
         
         block_num++;  // Move to the next block
     }
     
     delete[] blockDir;  // Clean up
 }
 
 /**
  * Find an entry in a directory by name.
  * 
  * Searches through a directory for an entry with the specified name
  * and returns a Node representing that entry if found.
  * 
  * @param dir Node representing the directory to search
  * @param name Name of the entry to find
  * @return Node representing the found entry, or nullptr if not found
  */
 Node* find_in_directory(Node* dir, const char* name) {
     debug_printf("DEBUG: find_in_directory: Looking for '%s' in directory (inode %u)\n", 
            name, dir->number);
     
     // Verify this is actually a directory
     if (!dir->is_dir()) {
         debug_printf("Error: Not a directory\n");
         return nullptr;
     }
     
     // Allocate a buffer for reading directory blocks
     char* blockDir = new char[1024 << supa->block_size];
     if (!blockDir) {
         debug_printf("Error: Failed to allocate memory for directory block\n");
         return nullptr;
     }
     
     uint32_t bytes_so_far = 0;  // Track how many bytes we've processed
     uint32_t block_num = 0;     // Start with the first block
     Node* result = nullptr;     // Will hold the found node
     
     // Process each block in the directory
     while (bytes_so_far < dir->node->size_of_iNode) {
         // Read one block of directory entries
         dir->read_block(block_num, blockDir);
         
         uint32_t offset = 0;  // Offset within this block
         
         // Process entries within this block
         while (offset < (1024 << supa->block_size)) {
             // Cast the current position to a directory entry structure
             dir_entry* entry = (dir_entry*)(blockDir + offset);
             
             // Check if we've reached the end of valid entries
             if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                 break;
             }
             
             // Extract the name from the entry
             char entry_name[256];
             uint32_t name_len = entry->name_length;
             if (name_len > 255) name_len = 255;  // Safety check
             
             // Copy the name characters (name starts after the 8-byte header)
             for (uint32_t i = 0; i < name_len; i++) {
                 entry_name[i] = blockDir[offset + 8 + i];
             }
             entry_name[name_len] = '\0';  // Null-terminate the string
             
             debug_printf("DEBUG: find_in_directory: Found entry '%s' (inode %u)\n", 
                    entry_name, entry->iNodeNum);
             
             // Check if this is the entry we're looking for
             if (streq_ext(name, entry_name)) {
                 // Found it! Create a Node object for this entry
                 debug_printf("DEBUG: find_in_directory: Match found!\n");
                 result = new Node(1024 << supa->block_size, entry->iNodeNum, dir->sd_adapter);
                 delete[] blockDir;
                 return result;
             }
             
             // Move to the next entry
             bytes_so_far += entry->size_entry;
             offset += entry->size_entry;
             
             // Check if we've reached the end of the block
             if (offset >= (1024 << supa->block_size)) {
                 break;
             }
         }
         
         block_num++;  // Move to the next block
     }
     
     debug_printf("DEBUG: find_in_directory: Entry '%s' not found\n", name);
     delete[] blockDir;  // Clean up
     return nullptr;     // Entry not found
 }
 
 /**
  * FILE OPERATIONS
  */
 
 /**
  * Read a file's contents into a buffer.
  * 
  * @param file Node representing the file to read
  * @param buffer Buffer to store the file contents
  * @param max_size Maximum number of bytes to read
  * @return Number of bytes read, or negative value on error
  */
 int read_file(Node* file, char* buffer, uint32_t max_size) {
     debug_printf("DEBUG: read_file: Reading file (inode %u, size %u)\n", 
            file->number, file->node->size_of_iNode);
     
     // Verify this is actually a file
     if (!file->is_file()) {
         debug_printf("Error: Not a file\n");
         return -1;
     }
     
     // Get the file size from the inode
     uint32_t file_size = file->node->size_of_iNode;
     
     // Check if the file is too large for the buffer
     if (file_size > max_size) {
         debug_printf("Warning: File too large for buffer, truncating\n");
         file_size = max_size;
     }
     
     // Read the file data using the BlockIO interface
     debug_printf("DEBUG: read_file: Reading %u bytes\n", file_size);
     int64_t bytes_read = file->read_all(0, file_size, buffer);
     
     // Null-terminate the buffer if there's room
     // This is helpful for text files but not strictly necessary
     if (bytes_read >= 0 && bytes_read < max_size) {
         buffer[bytes_read] = '\0';
     }
     
     debug_printf("DEBUG: read_file: Read %lld bytes\n", bytes_read);
     return bytes_read;
 }
 
 /**
  * FILE CREATION OPERATIONS
  */
 
 /**
  * Allocate a new inode from the inode bitmap.
  * 
  * Searches for a free inode in the same block group as the parent directory
  * for better locality of reference.
  * 
  * @param dir Node representing the parent directory
  * @return Allocated inode number, or 0 if no free inodes
  */
 uint32_t allocate_inode(Node* dir) {
     debug_printf("DEBUG: allocate_inode: Allocating new inode in block group %u\n", dir->block_group);
     
     // Use the same block group as the parent directory for locality
     uint32_t block_group = dir->block_group;
     
     // Find first free inode in this block group
     for (uint32_t i = 0; i < supa->num_iNode_pergroup; i++) {
         // Calculate the byte and bit position within the bitmap
         uint32_t byte_idx = i / 8;
         uint32_t bit_idx = i % 8;
         
         // Check if this bit is clear (0 = free, 1 = allocated)
         if ((cached_inode_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
             // Mark the inode as used in the bitmap
             cached_inode_bitmap[byte_idx] |= (1 << bit_idx);
             
             debug_printf("DEBUG: allocate_inode: Found free inode at index %u (byte %u, bit %u)\n", 
                    i, byte_idx, bit_idx);
                    
             // Mark the inode bitmap and BGDT as dirty
             inode_bitmap_dirty = true;
             bgdt_dirty = true;
             
             // Update free inode count in the BGDT
             bgdt->num_unallocated_iNodes--;
     
             // Delay writes — just mark dirty
             // Don't write bitmap or BGDT yet
             // Keep updated bitmap in memory if you need it for later
             
             // Calculate global inode number:
             // (block_group * num_iNode_pergroup) = first inode in this group
             // + i = offset within the group
             // + 1 = because inode numbers are 1-based
             uint32_t inode_num = (block_group * supa->num_iNode_pergroup) + i + 1;
             debug_printf("DEBUG: allocate_inode: Allocated inode %u\n", inode_num);
             return inode_num;
         }
     }
     
     debug_printf("DEBUG: allocate_inode: No free inodes found\n");
     return 0; // No free inodes
 }
 
 /**
  * Initialize a new inode with the given type.
  * 
  * Creates a new inode structure with default permissions and
  * writes it to disk.
  * 
  * @param inode_num Inode number to initialize
  * @param type Type of inode (0x8000 for file, 0x4000 for directory, etc.)
  * @param adapter Pointer to the SD adapter for disk access
  */
 void create_inode(uint32_t inode_num, uint16_t type, SDAdapter* adapter) {
     // Calculate inode location
     // Inode numbers are 1-based, so subtract 1 when computing offset
     uint32_t block_group = (inode_num - 1) / supa->num_iNode_pergroup;
     uint32_t index = (inode_num - 1) % supa->num_iNode_pergroup;
     
     // Calculate byte offset to the inode in the inode table
     uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                            index * supa->iNode_size;
     
     debug_printf("DEBUG: create_inode: inode %u, group %u, index %u, offset 0x%x\n", 
            inode_num, block_group, index, inode_offset);
     
     // Create a new inode structure
     iNode new_inode;
     zero_memory((uint8_t*)&new_inode, sizeof(iNode));
     
     // Set type and permissions:
     // - type parameter provides the file type (high 4 bits)
     // - 0x8180 = type | 0x0180 (0x0180 = 0644 octal = rw-r--r--)
     new_inode.types_plus_perm = type | 0x8180;
     new_inode.num_Hard_Links = 1;  // Initial hard link count is 1
     
     debug_printf("DEBUG: create_inode: Setting type+perm=0x%x, size=%u\n", 
            new_inode.types_plus_perm, new_inode.size_of_iNode);
     
     // Write the inode to disk
     int64_t write_result = adapter->write_all(inode_offset, sizeof(iNode), (char*)&new_inode);
     debug_printf("DEBUG: create_inode: Wrote inode to disk, result=%lld\n", write_result);
 }
 
 /**
  * Add a directory entry for a new file/directory.
  * 
  * Creates a new directory entry in the parent directory that
  * points to the specified inode.
  * 
  * @param dir Node representing the parent directory
  * @param name Name of the new entry
  * @param inode_num Inode number the entry should point to
  */
 void add_dir_entry(Node* dir, const char* name, uint32_t inode_num) {
     debug_printf("DEBUG: add_dir_entry: Adding entry '%s' (inode %u) to directory (inode %u)\n", 
            name, inode_num, dir->number);
     
     // Calculate entry size:
     // 1. Get the length of the filename
     uint32_t name_len = strlen_ext(name);
     
     // 2. Calculate total size (8-byte header + name)
     uint32_t entry_size = 8 + name_len;
     
     // 3. Round up to 4-byte boundary for alignment
     if (entry_size % 4 != 0) {
         entry_size += 4 - (entry_size % 4);
     }
     
     debug_printf("DEBUG: add_dir_entry: Entry size = %u bytes (name_len = %u)\n", 
            entry_size, name_len);
     
     // Find space in the directory to add the entry
     char* block_buf = new char[1024 << supa->block_size];
     bool found_space = false;
     uint32_t block_num = 0;
     uint32_t offset = 0;
     
     // Try to find space in existing blocks first
     // Only check direct blocks (0-11)
     while (block_num < 12 && !found_space) {
         debug_printf("DEBUG: add_dir_entry: Checking block %u\n", block_num);
         
         if (dir->node->directLinked[block_num] == 0) {
             // This block isn't allocated yet, allocate it
             debug_printf("DEBUG: add_dir_entry: Block not allocated, allocating new block\n");
             uint32_t new_block = dir->allocate_block();
             if (new_block == 0) {
                 debug_printf("add_dir_entry: Failed to allocate new block\n");
                 delete[] block_buf;
                 return;
             }
             
             debug_printf("DEBUG: add_dir_entry: Allocated block %u\n", new_block);
             dir->node->directLinked[block_num] = new_block;
             dir_inode_dirty = true;
             found_space = true;
         } else {
             // Read existing block to look for space
             debug_printf("DEBUG: add_dir_entry: Reading existing block %u\n", 
                   dir->node->directLinked[block_num]);
             dir->read_block(block_num, block_buf);
             
             // Scan through existing entries to find space
             uint32_t pos = 0;
             while (pos < (1024 << supa->block_size)) {
                 dir_entry* entry = (dir_entry*)(block_buf + pos);
                 
                 debug_printf("DEBUG: add_dir_entry: Checking entry at offset %u: inode=%u, size=%u\n", 
                        pos, entry->iNodeNum, entry->size_entry);
                 
                 // Check if this is an empty/unused entry
                 if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                     // Found empty space
                     debug_printf("DEBUG: add_dir_entry: Found empty space at offset %u\n", pos);
                     offset = pos;
                     found_space = true;
                     break;
                 }
                 
                 // Move to next entry
                 pos += entry->size_entry;
                 if (pos >= (1024 << supa->block_size)) {
                     debug_printf("DEBUG: add_dir_entry: Reached end of block\n");
                     break;
                 }
             }
         }
         
         if (found_space) break;
         block_num++;
     }
     
     if (!found_space) {
         debug_printf("DEBUG: add_dir_entry: No space found in directory\n");
         delete[] block_buf;
         return;
     }
     
     // Create the directory entry
     debug_printf("DEBUG: add_dir_entry: Creating entry at block %u, offset %u\n", 
            block_num, offset);
            
     // Cast the appropriate position in the buffer to a directory entry
     dir_entry* new_entry = (dir_entry*)(block_buf + offset);
     
     // Initialize the entry fields
     new_entry->iNodeNum = inode_num;
     new_entry->size_entry = entry_size;
     new_entry->name_length = name_len;
     new_entry->type_indicator = 1;
     // Copy the name to the entry
    for (uint32_t i = 0; i < name_len; i++) {
        *(block_buf + offset + 8 + i) = name[i];
    }
    
    // Write the updated block back to disk
    debug_printf("DEBUG: add_dir_entry: Writing block back to disk\n");
    dir->sd_adapter->write_block(dir->node->directLinked[block_num], block_buf, 0, 1024 << supa->block_size);
    
    // Update directory size if needed
    // Calculate end position:
    // (block_num * block_size) = start of block in bytes
    // + offset = position within block
    // + entry_size = bytes used by the entry
    uint32_t end_pos = (block_num * (1024 << supa->block_size)) + offset + entry_size;
    
    // If this entry extends beyond the current directory size, update the size
    if (end_pos > dir->node->size_of_iNode) {
        debug_printf("DEBUG: add_dir_entry: Updating directory size from %u to %u\n", 
               dir->node->size_of_iNode, end_pos);
        dir->node->size_of_iNode = end_pos;
        dir_inode_dirty = true;
    }
    
    delete[] block_buf;
}

/**
 * Create a new file in a directory.
 * 
 * Performs the complete process of creating a new file:
 * 1. Allocates an inode
 * 2. Initializes the inode as a file
 * 3. Adds a directory entry in the parent directory
 * 4. Updates filesystem metadata
 * 
 * @param dir Node representing the parent directory
 * @param name Name of the new file
 * @return Node representing the new file, or nullptr on error
 */
Node* create_file(Node* dir, const char* name) {
    debug_printf("DEBUG: create_file: Creating file '%s' in directory (inode %u)\n", name, dir->number);
    
    // Verify this is actually a directory
    if (!dir->is_dir()) {
        debug_printf("DEBUG: create_file: Not a directory\n");
        return nullptr;
    }
    
    // Check if file already exists
    Node* existing = find_in_directory(dir, name);
    if (existing) {
        debug_printf("DEBUG: create_file: File already exists (inode %u)\n", existing->number);
        delete existing;
        return nullptr;
    }
    
    // Allocate an inode for the new file
    debug_printf("DEBUG: create_file: Allocating new inode\n");
    uint32_t inode_num = allocate_inode(dir);
    if (inode_num == 0) {
        debug_printf("DEBUG: create_file: Failed to allocate inode\n");
        return nullptr;
    }
    debug_printf("DEBUG: create_file: Allocated inode %u\n", inode_num);
    
    // Create the inode as a regular file
    // 0x8000 = regular file type in the high 4 bits of the mode field
    debug_printf("DEBUG: create_file: Creating inode with type 0x8000 (file)\n");
    create_inode(inode_num, 0x8000, dir->sd_adapter);
    
    // Add directory entry in the parent directory
    debug_printf("DEBUG: create_file: Adding directory entry\n");
    add_dir_entry(dir, name, inode_num);
    
    // Return the new node
    debug_printf("DEBUG: create_file: Creating Node object for new file\n");
    
    // ----- FLUSH METADATA BASED ON DIRTY FLAGS -----

    // Flush inode bitmap if dirty
    if (inode_bitmap_dirty) {
        debug_printf("DEBUG: Flushing inode bitmap to disk\n");
        dir->sd_adapter->write_block(bgdt->bit_map_iNode_address, 
                                   (char*)cached_inode_bitmap, 
                                   0, 
                                   1024 << supa->block_size);
        inode_bitmap_dirty = false;
    }

    // Flush block bitmap if dirty
    if (block_bitmap_dirty) {
        debug_printf("DEBUG: Flushing block bitmap to disk\n");
        dir->sd_adapter->write_block(bgdt->bit_map_block_address, 
                                   (char*)cached_block_bitmap, 
                                   0, 
                                   1024 << supa->block_size);
        block_bitmap_dirty = false;
    }

    // Flush BGDT if dirty
    if (bgdt_dirty) {
        debug_printf("DEBUG: Flushing BGDT to disk\n");
        uint32_t bgdt_offset = BGDT_index * (1024 << supa->block_size) +
                              (dir->block_group * sizeof(BGDT));
        dir->sd_adapter->write_all(bgdt_offset, sizeof(BGDT), (char*)bgdt);
        bgdt_dirty = false;
    }

    // Flush directory inode if dirty
    if (dir_inode_dirty) {
        debug_printf("DEBUG: Flushing parent directory inode to disk\n");
        dir->update_inode_on_disk();
        dir_inode_dirty = false;
    }

    // Create and return a Node for the new file
    return new Node(1024 << supa->block_size, inode_num, dir->sd_adapter);
}