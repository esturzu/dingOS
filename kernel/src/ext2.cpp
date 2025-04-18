#include "ext2.h"
/*
 * EXT2 FILESYSTEM IMPLEMENTATION
 *
 * This file implements the ext2 filesystem operations defined in ext2.h.
 * It provides functionality to:
 * - Initialize the filesystem
 * - List directory contents
 * - Find files and directories
 * - Read and write file contents
 */

// Global variables
super_block* supa = nullptr;    // Superblock
BGDT* bgdt = nullptr;           // Block Group Descriptor Table
uint32_t SB_offset = 1024;      // Superblock offset (always 1024 bytes)
uint32_t BGDT_index = 0;        // Block index of the BGDT
bool inode_bitmap_dirty = false;
bool block_bitmap_dirty = false;
bool bgdt_dirty = false;
bool dir_inode_dirty = false;
// uint8_t* cached_inode_bitmap = new uint8_t[1024 << supa->block_size];
// uint8_t* cached_block_bitmap = new uint8_t[1024 << supa->block_size];
uint8_t* cached_inode_bitmap = nullptr;
uint8_t* cached_block_bitmap = nullptr;



/*
 * UTILITY FUNCTIONS
 */

// Dump a block's contents for debugging
void dump_blocks(SDAdapter* adapter, uint32_t block_start, uint32_t count) {
    const uint32_t block_size = adapter->block_size;
    char* buffer = new char[block_size];

    for (uint32_t i = 0; i < count; i++) {
        uint32_t block_num = block_start + i;
        adapter->read_block(block_num, buffer);

        printf("Block %u (size: %u bytes):\n", block_num, block_size);
        for (uint32_t offset = 0; offset < block_size; offset += 64) {
            printf("  %04x: ", offset);
            for (uint32_t j = 0; j < 64 && (offset + j) < block_size; j++) {
                printf("%02x ", (uint8_t)buffer[offset + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    delete[] buffer;
}


// Compare two strings for equality
bool streq_ext(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;  // Both strings end at the same position
}

// Get string length
uint32_t strlen_ext(const char* str) {
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

// Simple memory zeroing function (replacement for memset)
void zero_memory(void* buffer, uint32_t size) {
    char* p = (char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/*
 * NODE METHODS IMPLEMENTATION
 */

// Write a block to this node
void Node::write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) {
    printf("DEBUG: Node::write_block: inode %u, block %u, offset %u, size %u\n", 
           number, block_number, offset, n);
    
    // Only support direct blocks for now
    if (block_number >= 12) {
        printf("DEBUG: Node::write_block: Only direct blocks (0-11) supported in this version\n");
        return;
    }
    
    // Check if the block is already allocated
    if (node->directLinked[block_number] == 0) {
        printf("DEBUG: Node::write_block: Block not allocated, allocating new block\n");
        
        // Need to allocate a new block
        uint32_t new_block = allocate_block();
        if (new_block == 0) {
            printf("DEBUG: Node::write_block: Failed to allocate new block\n");
            return;
        }
        
        printf("DEBUG: Node::write_block: Allocated block %u\n", new_block);
        node->directLinked[block_number] = new_block;
    }
    
    printf("DEBUG: Node::write_block: Writing to physical block %u\n", 
           node->directLinked[block_number]);
    
    // Write the data to the block
    sd_adapter->write_block(node->directLinked[block_number], buffer, offset, n);
    
    // Update file size if needed
    uint32_t end_pos = (block_number * block_size) + offset + n;
    printf("DEBUG: Node::write_block: Current size=%u, new end_pos=%u\n", 
           node->size_of_iNode, end_pos);
    
    if (end_pos > node->size_of_iNode) {
        printf("DEBUG: Node::write_block: Updating inode size from %u to %u\n", 
               node->size_of_iNode, end_pos);
        node->size_of_iNode = end_pos;
        update_inode_on_disk();
    }
}

// Update inode on disk after changes
void Node::update_inode_on_disk() {
    printf("DEBUG: Node::update_inode_on_disk: Saving inode %u (size=%u, type=0x%x)\n", 
           number, node->size_of_iNode, node->types_plus_perm);
    
    uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                           index * supa->iNode_size;
    
    printf("DEBUG: Node::update_inode_on_disk: Writing to offset 0x%x, size=%u\n", 
           inode_offset, sizeof(iNode));
    int64_t result = sd_adapter->write_all(inode_offset, sizeof(iNode), (char*)node);
    printf("DEBUG: Node::update_inode_on_disk: Write result=%lld\n", result);
}

// Find and allocate a free block
uint32_t Node::allocate_block() {
    printf("DEBUG: Node::allocate_block: Finding free block for inode %u\n", number);
    
    // Detailed bitmap tracking
    uint32_t total_blocks = supa->num_Blocks;
    uint32_t free_blocks_in_bitmap = 0;
    uint32_t first_free_block = 0;
    
    // Scan through bitmap to count free blocks and find first free block
    for (uint32_t i = 0; i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if ((cached_block_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            free_blocks_in_bitmap++;
            if (first_free_block == 0) {
                first_free_block = i + 1; // Block numbers start at 1
            }
        }
    }
    
    
    // Check for available free blocks
    if (first_free_block == 0) {
        printf("ERROR: No free blocks available!\n");
        return 0;
    }
    
    // Find first free block
    for (uint32_t i = 0; i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if ((cached_block_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            // Mark as used
            cached_block_bitmap[byte_idx] |= (1 << bit_idx);
            
            printf("DEBUG: Found free block %u (byte %u, bit %u)\n", 
                   i + 1, byte_idx, bit_idx);
            block_bitmap_dirty = true;
            bgdt_dirty = true;
            return i + 1; // Block numbers start at 1
        }
    }
    
    printf("ERROR: No free blocks found despite initial count\n");
    return 0; // No free blocks
}

/*
 * DIRECTORY OPERATIONS
 */

// List all entries in a directory
void list_directory(Node* dir) {
    if (!dir->is_dir()) {
        printf("Error: Not a directory\n");
        return;
    }
    
    printf("Directory listing (inode %u):\n", dir->number);
    
    // Allocate a buffer for a directory block
    // Size is determined by the filesystem's block size
    char* blockDir = new char[1024 << supa->block_size];
    if (!blockDir) {
        printf("Error: Failed to allocate memory for directory block\n");
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
            // Cast the current position to a directory entry
            dir_entry* entry = (dir_entry*)(blockDir + offset);
            if (entry->size_entry == 0 || entry->size_entry > (1024 << supa->block_size) - offset) {
                printf("Corrupt directory entry at offset %u: size_entry=%u\n", offset, entry->size_entry);
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
            for (uint32_t i = 0; i < name_len; i++) {
                entry_name[i] = blockDir[offset + 8 + i];  // 8 bytes for the fixed header fields
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
            printf("  %8u  %8u  %c  %s\n", 
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

// Find an entry in a directory by name
Node* find_in_directory(Node* dir, const char* name) {
    printf("DEBUG: find_in_directory: Looking for '%s' in directory (inode %u)\n", 
           name, dir->number);
    
    if (!dir->is_dir()) {
        printf("Error: Not a directory\n");
        return nullptr;
    }
    
    // Allocate a buffer for a directory block
    char* blockDir = new char[1024 << supa->block_size];
    if (!blockDir) {
        printf("Error: Failed to allocate memory for directory block\n");
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
            // Cast the current position to a directory entry
            dir_entry* entry = (dir_entry*)(blockDir + offset);
            
            // Check if we've reached the end of valid entries
            if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                break;
            }
            
            // Extract the name from the entry
            char entry_name[256];
            uint32_t name_len = entry->name_length;
            if (name_len > 255) name_len = 255;  // Safety check
            
            // Copy the name characters
            for (uint32_t i = 0; i < name_len; i++) {
                entry_name[i] = blockDir[offset + 8 + i];  // 8 bytes for the fixed header fields
            }
            entry_name[name_len] = '\0';  // Null-terminate the string
            
            printf("DEBUG: find_in_directory: Found entry '%s' (inode %u)\n", 
                   entry_name, entry->iNodeNum);
            
            // Check if this is the entry we're looking for
            if (streq_ext(name, entry_name)) {
                // Found it! Create a node for this entry
                printf("DEBUG: find_in_directory: Match found!\n");
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
    
    printf("DEBUG: find_in_directory: Entry '%s' not found\n", name);
    delete[] blockDir;  // Clean up
    return nullptr;     // Entry not found
}

/*
 * FILE OPERATIONS
 */

// Read a file's contents into a buffer
int read_file(Node* file, char* buffer, uint32_t max_size) {
    printf("DEBUG: read_file: Reading file (inode %u, size %u)\n", 
           file->number, file->node->size_of_iNode);
    
    if (!file->is_file()) {
        printf("Error: Not a file\n");
        return -1;
    }
    
    // Get the file size
    uint32_t file_size = file->node->size_of_iNode;
    
    // Check if the file is too large for the buffer
    if (file_size > max_size) {
        printf("Warning: File too large for buffer, truncating\n");
        file_size = max_size;
    }
    
    // Read the file data using the BlockIO interface
    printf("DEBUG: read_file: Reading %u bytes\n", file_size);
    int64_t bytes_read = file->read_all(0, file_size, buffer);
    
    // Null-terminate the buffer if there's room
    if (bytes_read >= 0 && bytes_read < max_size) {
        buffer[bytes_read] = '\0';
    }
    
    printf("DEBUG: read_file: Read %lld bytes\n", bytes_read);
    return bytes_read;
}

/*
 * FILE CREATION OPERATIONS
 */

// Allocate a new inode
uint32_t allocate_inode(Node* dir) {
    printf("DEBUG: allocate_inode: Allocating new inode in block group %u\n", dir->block_group);
    
    // Use the same block group as the parent directory for locality
    uint32_t block_group = dir->block_group;
    
    // Find first free inode in this block group
    for (uint32_t i = 0; i < supa->num_iNode_pergroup; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        if ((cached_inode_bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            cached_inode_bitmap[byte_idx] |= (1 << bit_idx);
            printf("DEBUG: allocate_inode: Found free inode at index %u (byte %u, bit %u)\n", 
                   i, byte_idx, bit_idx);
            inode_bitmap_dirty = true;
            bgdt_dirty = true;
            bgdt->num_unallocated_iNodes--;
    
            // Delay writes — just mark dirty
            // Don't write bitmap or BGDT yet
            // Keep updated bitmap in memory if you need it for later
            uint32_t inode_num = (block_group * supa->num_iNode_pergroup) + i + 1; // inode numbers start at 1
            printf("DEBUG: allocate_inode: Allocated inode %u\n", inode_num);
            return inode_num;
        }
        
    }
    
    printf("DEBUG: allocate_inode: No free inodes found\n");
    return 0; // No free inodes
}

// Initialize a new inode
void create_inode(uint32_t inode_num, uint16_t type, SDAdapter* adapter) {
    // Calculate inode location
    uint32_t block_group = (inode_num - 1) / supa->num_iNode_pergroup;
    uint32_t index = (inode_num - 1) % supa->num_iNode_pergroup;
    uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                           index * supa->iNode_size;
    
    printf("DEBUG: create_inode: inode %u, group %u, index %u, offset 0x%x\n", 
           inode_num, block_group, index, inode_offset);
    
    // Create a new inode structure
    iNode new_inode;
    zero_memory((uint8_t*)&new_inode, sizeof(iNode));
    
    // Set type and permissions (file with read/write for owner)
    // Explicitly set the full 16-bit value to include type
    new_inode.types_plus_perm = type | 0x8180; // 0x8000 for file + 0644 permissions
    new_inode.num_Hard_Links = 1;
    
    printf("DEBUG: create_inode: Setting type+perm=0x%x, size=%u\n", 
           new_inode.types_plus_perm, new_inode.size_of_iNode);
    
    // Write the inode to disk
    int64_t write_result = adapter->write_all(inode_offset, sizeof(iNode), (char*)&new_inode);
    printf("DEBUG: create_inode: Wrote inode to disk, result=%lld\n", write_result);
}

// Add a directory entry
void add_dir_entry(Node* dir, const char* name, uint32_t inode_num) {
    printf("DEBUG: add_dir_entry: Adding entry '%s' (inode %u) to directory (inode %u)\n", 
           name, inode_num, dir->number);
    
    // Calculate entry size (header + name length, rounded to 4-byte boundary)
    uint32_t name_len = strlen_ext(name);
    
    uint32_t entry_size = 8 + name_len; // 8 bytes for header
    if (entry_size % 4 != 0) {
        entry_size += 4 - (entry_size % 4); // Round up to 4-byte boundary
    }
    
    printf("DEBUG: add_dir_entry: Entry size = %u bytes (name_len = %u)\n", 
           entry_size, name_len);
    
    // Find space in the directory
    char* block_buf = new char[1024 << supa->block_size];
    bool found_space = false;
    uint32_t block_num = 0;
    uint32_t offset = 0;
    
    // Try to find space in existing blocks first
    while (block_num < 12 && !found_space) { // Only check direct blocks
        printf("DEBUG: add_dir_entry: Checking block %u\n", block_num);
        
        if (dir->node->directLinked[block_num] == 0) {
            // This block isn't allocated yet
            printf("DEBUG: add_dir_entry: Block not allocated, allocating new block\n");
            uint32_t new_block = dir->allocate_block();
            if (new_block == 0) {
                printf("add_dir_entry: Failed to allocate new block\n");
                delete[] block_buf;
                return;
            }
            
            printf("DEBUG: add_dir_entry: Allocated block %u\n", new_block);
            dir->node->directLinked[block_num] = new_block;
            dir_inode_dirty = true;
            // offset = 0;
            found_space = true;
        } else {
            // Read existing block
            printf("DEBUG: add_dir_entry: Reading existing block %u\n", 
                  dir->node->directLinked[block_num]);
            dir->read_block(block_num, block_buf);
            
            // Scan through entries to find space
            uint32_t pos = 0;
            while (pos < (1024 << supa->block_size)) {
                dir_entry* entry = (dir_entry*)(block_buf + pos);
                
                printf("DEBUG: add_dir_entry: Checking entry at offset %u: inode=%u, size=%u\n", 
                       pos, entry->iNodeNum, entry->size_entry);
                
                if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                    // Found empty space
                    printf("DEBUG: add_dir_entry: Found empty space at offset %u\n", pos);
                    offset = pos;
                    found_space = true;
                    break;
                }
                
                // Move to next entry
                pos += entry->size_entry;
                if (pos >= (1024 << supa->block_size)) {
                    printf("DEBUG: add_dir_entry: Reached end of block\n");
                    break;
                }
            }
        }
        
        if (found_space) break;
        block_num++;
    }
    
    if (!found_space) {
        printf("DEBUG: add_dir_entry: No space found in directory\n");
        delete[] block_buf;
        return;
    }
    
    // Create the directory entry
    printf("DEBUG: add_dir_entry: Creating entry at block %u, offset %u\n", 
           block_num, offset);
    dir_entry* new_entry = (dir_entry*)(block_buf + offset);
    new_entry->iNodeNum = inode_num;
    new_entry->size_entry = entry_size;
    new_entry->name_length = name_len;
    new_entry->type_indicator = 1; // Regular file
    
    // Copy the name
    for (uint32_t i = 0; i < name_len; i++) {
        *(block_buf + offset + 8 + i) = name[i];
    }
    
    // Write the updated block back to disk
    printf("DEBUG: add_dir_entry: Writing block back to disk\n");
    dir->sd_adapter->write_block(dir->node->directLinked[block_num], block_buf, 0, 1024 << supa->block_size);
    
    // Update directory size if needed
    uint32_t end_pos = (block_num * (1024 << supa->block_size)) + offset + entry_size;
    if (end_pos > dir->node->size_of_iNode) {
        printf("DEBUG: add_dir_entry: Updating directory size from %u to %u\n", 
               dir->node->size_of_iNode, end_pos);
        dir->node->size_of_iNode = end_pos;
        dir_inode_dirty = true;
    }
    
    delete[] block_buf;
}

// Create a new file
Node* create_file(Node* dir, const char* name) {
    printf("DEBUG: create_file: Creating file '%s' in directory (inode %u)\n", name, dir->number);
    
    if (!dir->is_dir()) {
        printf("DEBUG: create_file: Not a directory\n");
        return nullptr;
    }
    
    // Check if file already exists
    Node* existing = find_in_directory(dir, name);
    if (existing) {
        printf("DEBUG: create_file: File already exists (inode %u)\n", existing->number);
        delete existing;
        return nullptr;
    }
    
    // Allocate an inode
    printf("DEBUG: create_file: Allocating new inode\n");
    uint32_t inode_num = allocate_inode(dir);
    if (inode_num == 0) {
        printf("DEBUG: create_file: Failed to allocate inode\n");
        return nullptr;
    }
    printf("DEBUG: create_file: Allocated inode %u\n", inode_num);
    
    // Create the inode (file type)
    printf("DEBUG: create_file: Creating inode with type 0x8000 (file)\n");
    create_inode(inode_num, 0x8000, dir->sd_adapter); // 0x8000 = regular file type
    
    // Add directory entry
    printf("DEBUG: create_file: Adding directory entry\n");
    add_dir_entry(dir, name, inode_num);
    
    // Return the new node
    printf("DEBUG: create_file: Creating Node object for new file\n");
        // ----- FLUSH METADATA BASED ON DIRTY FLAGS -----

    // Flush inode bitmap if dirty
    if (inode_bitmap_dirty) {
        printf("DEBUG: Flushing inode bitmap to disk\n");
        dir->sd_adapter->write_block(bgdt->bit_map_iNode_address, (char*)cached_inode_bitmap, 0, 1024 << supa->block_size);
        inode_bitmap_dirty = false;
    }

    // Flush block bitmap if dirty
    if (block_bitmap_dirty) {
        printf("DEBUG: Flushing block bitmap to disk\n");
        dir->sd_adapter->write_block(bgdt->bit_map_block_address, (char*)cached_block_bitmap, 0, 1024 << supa->block_size);
        block_bitmap_dirty = false;
    }

    // Flush BGDT if dirty
    if (bgdt_dirty) {
        printf("DEBUG: Flushing BGDT to disk\n");
        uint32_t bgdt_offset = BGDT_index * (1024 << supa->block_size) +
                               (dir->block_group * sizeof(BGDT));
        dir->sd_adapter->write_all(bgdt_offset, sizeof(BGDT), (char*)bgdt);
        bgdt_dirty = false;
    }

    // Flush directory inode if dirty
    if (dir_inode_dirty) {
        printf("DEBUG: Flushing parent directory inode to disk\n");
        dir->update_inode_on_disk();
        dir_inode_dirty = false;
    }

    return new Node(1024 << supa->block_size, inode_num, dir->sd_adapter);
}

/*
 * INITIALIZATION
 */


// Initialize the ext2 filesystem
void init_ext2() {
    // Initialize the SD card
    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD card initialization failed!\n");
        return;
    }
    
    printf("SD card initialized successfully\n");
    
    // Create an SD adapter with initial block size
    SDAdapter* adapter = new SDAdapter(1024);
    
    // Read the superblock
    supa = new super_block();
    adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
    
    // Update the adapter with the correct block size
    uint32_t fs_block_size = 1024 << supa->block_size;
    adapter = new SDAdapter(fs_block_size);
    
    // Read the block group descriptor table
    bgdt = new BGDT();
    
    // Calculate BGDT location based on block size
    if ((1024 << supa->block_size) == 1024) {
        BGDT_index = 2;  // For 1K blocks, BGDT starts at block 2
    } else {
        BGDT_index = 1;  // For larger blocks, BGDT starts at block 1
    }
    
    // Read the BGDT
    adapter->read_all(BGDT_index * fs_block_size, sizeof(BGDT), (char*)bgdt);
    
    // Create an ext2 filesystem instance
    Ext2* fs = new Ext2(adapter);
    
    // List the root directory contents
    printf("Listing root directory contents:\n");
    list_directory(fs->root);
    
    // Print root directory information
    if (fs->root && fs->root->is_dir()) {
        printf("Root directory found (inode %u)\n", fs->root->number);
    } else {
        printf("Error: Root is not a directory or not found\n");
    }
    
    // Don't delete fs here if you want to keep using it
    // The initialization function should probably return the filesystem
    // or store it in a global variable
}


