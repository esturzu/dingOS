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


/*
 * UTILITY FUNCTIONS
 */

// Dump a block's contents for debugging
void dump_block(uint32_t block_number) {
    uint8_t buffer[512];
    uint32_t bytes = SD::read(block_number, 1, buffer);
    
    printf("Block %u (first 64 bytes):\n", block_number);
    for (int i = 0; i < 64 && i < bytes; i++) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
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
    // Only support direct blocks for now
    if (block_number >= 12) {
        printf("Node::write_block: Only direct blocks (0-11) supported in this version\n");
        return;
    }
    
    // Check if the block is already allocated
    if (node->directLinked[block_number] == 0) {
        // Need to allocate a new block
        uint32_t new_block = allocate_block();
        if (new_block == 0) {
            printf("Node::write_block: Failed to allocate new block\n");
            return;
        }
        node->directLinked[block_number] = new_block;
        
        // Update inode on disk
        update_inode_on_disk();
    }
    
    // Write the data to the block
    sd_adapter->write_block(node->directLinked[block_number], buffer, offset, n);
    
    // Update file size if needed
    uint32_t new_size = (block_number * block_size) + offset + n;
    if (new_size > node->size_of_iNode) {
        node->size_of_iNode = new_size;
        update_inode_on_disk();
    }
}

// Update inode on disk after changes
void Node::update_inode_on_disk() {
    uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                           index * supa->iNode_size;
    sd_adapter->write_all(inode_offset, sizeof(iNode), (char*)node);
}

// Find and allocate a free block
uint32_t Node::allocate_block() {
    // Read the block bitmap
    uint8_t* bitmap = new uint8_t[1024 << supa->block_size];
    sd_adapter->read_block(bgdt->bit_map_block_address, (char*)bitmap);
    
    // Find first free block in this block group
    for (uint32_t i = 0; i < (1024 << supa->block_size) * 8; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            // Mark as used
            bitmap[byte_idx] |= (1 << bit_idx);
            
            // Write bitmap back
            sd_adapter->write_block(bgdt->bit_map_block_address, (char*)bitmap, 0, 1024 << supa->block_size);
            
            // Update BGDT
            bgdt->num_unallocated_blocks--;
            
            // Update BGDT on disk
            uint32_t bgdt_offset = BGDT_index * (1024 << supa->block_size) + (block_group * sizeof(BGDT));
            sd_adapter->write_all(bgdt_offset, sizeof(BGDT), (char*)bgdt);
            
            delete[] bitmap;
            return i + 1; // Block numbers start at 1
        }
    }
    
    delete[] bitmap;
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
            
            // Check if this is the entry we're looking for
            if (streq_ext(name, entry_name)) {
                // Found it! Create a node for this entry
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
    
    delete[] blockDir;  // Clean up
    return nullptr;     // Entry not found
}

/*
 * FILE OPERATIONS
 */

// Read a file's contents into a buffer
int read_file(Node* file, char* buffer, uint32_t max_size) {
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
    int64_t bytes_read = file->read_all(0, file_size, buffer);
    
    // Null-terminate the buffer if there's room
    if (bytes_read >= 0 && bytes_read < max_size) {
        buffer[bytes_read] = '\0';
    }
    
    return bytes_read;
}

/*
 * FILE CREATION OPERATIONS
 */

// Allocate a new inode
uint32_t allocate_inode(Node* dir) {
    // Use the same block group as the parent directory for locality
    uint32_t block_group = dir->block_group;
    
    // Read the inode bitmap
    uint8_t* bitmap = new uint8_t[1024 << supa->block_size];
    dir->sd_adapter->read_block(bgdt->bit_map_iNode_address, (char*)bitmap);
    
    // Find first free inode in this block group
    for (uint32_t i = 0; i < supa->num_iNode_pergroup; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if ((bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            // Mark as used
            bitmap[byte_idx] |= (1 << bit_idx);
            
            // Write bitmap back
            dir->sd_adapter->write_block(bgdt->bit_map_iNode_address, (char*)bitmap, 0, 1024 << supa->block_size);
            
            // Update BGDT
            bgdt->num_unallocated_iNodes--;
            
            // Update BGDT on disk
            uint32_t bgdt_offset = BGDT_index * (1024 << supa->block_size) + (block_group * sizeof(BGDT));
            dir->sd_adapter->write_all(bgdt_offset, sizeof(BGDT), (char*)bgdt);
            
            delete[] bitmap;
            return (block_group * supa->num_iNode_pergroup) + i + 1; // inode numbers start at 1
        }
    }
    
    delete[] bitmap;
    return 0; // No free inodes
}

// Initialize a new inode
void create_inode(uint32_t inode_num, uint16_t type, SDAdapter* adapter) {
    // Calculate inode location
    uint32_t block_group = (inode_num - 1) / supa->num_iNode_pergroup;
    uint32_t index = (inode_num - 1) % supa->num_iNode_pergroup;
    uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                           index * supa->iNode_size;
    
    // Create a new inode structure
    iNode new_inode;
    zero_memory(&new_inode, sizeof(iNode)); // Replace memset with zero_memory
    
    // Set type and permissions (file with read/write for owner)
    new_inode.types_plus_perm = type | 0x1A4; // 0644 permissions
    new_inode.num_Hard_Links = 1;
    
    // Write the inode to disk
    adapter->write_all(inode_offset, sizeof(iNode), (char*)&new_inode);
}

// Add a directory entry
void add_dir_entry(Node* dir, const char* name, uint32_t inode_num) {
    // Calculate entry size (header + name length, rounded to 4-byte boundary)
    uint32_t name_len = strlen_ext(name);
    
    uint32_t entry_size = 8 + name_len; // 8 bytes for header
    if (entry_size % 4 != 0) {
        entry_size += 4 - (entry_size % 4); // Round up to 4-byte boundary
    }
    
    // Find space in the directory
    char* block_buf = new char[1024 << supa->block_size];
    bool found_space = false;
    uint32_t block_num = 0;
    uint32_t offset = 0;
    
    // Try to find space in existing blocks first
    while (block_num < 12 && !found_space) { // Only check direct blocks
        if (dir->node->directLinked[block_num] == 0) {
            // This block isn't allocated yet
            uint32_t new_block = dir->allocate_block();
            if (new_block == 0) {
                printf("add_dir_entry: Failed to allocate new block\n");
                delete[] block_buf;
                return;
            }
            
            dir->node->directLinked[block_num] = new_block;
            dir->update_inode_on_disk();
            
            // Clear the new block
            zero_memory(block_buf, 1024 << supa->block_size);
            
            // New entry goes at the start
            offset = 0;
            found_space = true;
        } else {
            // Read existing block
            dir->read_block(block_num, block_buf);
            
            // Scan through entries to find space
            uint32_t pos = 0;
            while (pos < (1024 << supa->block_size)) {
                dir_entry* entry = (dir_entry*)(block_buf + pos);
                
                if (entry->iNodeNum == 0 || entry->size_entry == 0) {
                    // Found empty space
                    offset = pos;
                    found_space = true;
                    break;
                }
                
                // Move to next entry
                pos += entry->size_entry;
                if (pos >= (1024 << supa->block_size)) break;
            }
        }
        
        if (found_space) break;
        block_num++;
    }
    
    if (!found_space) {
        printf("add_dir_entry: No space found in directory\n");
        delete[] block_buf;
        return;
    }
    
    // Create the directory entry
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
    dir->sd_adapter->write_block(dir->node->directLinked[block_num], block_buf, 0, 1024 << supa->block_size);
    
    // Update directory size if needed
    uint32_t end_pos = (block_num * (1024 << supa->block_size)) + offset + entry_size;
    if (end_pos > dir->node->size_of_iNode) {
        dir->node->size_of_iNode = end_pos;
        dir->update_inode_on_disk();
    }
    
    delete[] block_buf;
}

// Create a new file
Node* create_file(Node* dir, const char* name) {
    if (!dir->is_dir()) {
        printf("create_file: Not a directory\n");
        return nullptr;
    }
    
    // Check if file already exists
    Node* existing = find_in_directory(dir, name);
    if (existing) {
        printf("create_file: File already exists\n");
        delete existing;
        return nullptr;
    }
    
    // Allocate an inode
    uint32_t inode_num = allocate_inode(dir);
    if (inode_num == 0) {
        printf("create_file: Failed to allocate inode\n");
        return nullptr;
    }
    
    // Create the inode (file type)
    create_inode(inode_num, 0x8000, dir->sd_adapter); // 0x8000 = regular file type
    
    // Add directory entry
    add_dir_entry(dir, name, inode_num);
    
    // Return the new node
    return new Node(1024 << supa->block_size, inode_num, dir->sd_adapter);
}

/*
 * INITIALIZATION AND TESTING
 */

// Test the directory traversal functionality
void test_directory_traversal() {
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
    
    // Test the root directory
    if (fs->root && fs->root->is_dir()) {
        printf("Root directory found (inode %u)\n", fs->root->number);
        
        // List the contents of the root directory
        list_directory(fs->root);
        
        // Try to find and read a test file
        const char* test_filename = "hello.txt";
        printf("Looking for file: %s\n", test_filename);
        
        Node* test_file = find_in_directory(fs->root, test_filename);
        
        if (test_file) {
            printf("Found file '%s' (inode %u)\n", test_filename, test_file->number);
            
            // Read and display the file contents
            char buffer[1024];
            int bytes_read = read_file(test_file, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                printf("File contents (%d bytes):\n%s\n", bytes_read, buffer);
            } else {
                printf("Error reading file or empty file\n");
            }
            
            delete test_file;
        } else {
            printf("File '%s' not found\n", test_filename);
        }
    } else {
        printf("Error: Root is not a directory or not found\n");
    }
    
    // Clean up
    delete fs;
    delete adapter;
}

// Test file creation and writing
void test_file_creation() {
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
    
    // Calculate BGDT location
    if ((1024 << supa->block_size) == 1024) {
        BGDT_index = 2;
    } else {
        BGDT_index = 1;
    }
    
    // Read the BGDT
    adapter->read_all(BGDT_index * fs_block_size, sizeof(BGDT), (char*)bgdt);
    
    // Create an ext2 filesystem instance
    Ext2* fs = new Ext2(adapter);
    
    printf("Creating test file in root directory...\n");
    
    // Create a test file
    const char* test_filename = "dingos_test.txt";
    Node* test_file = create_file(fs->root, test_filename);
    
    if (test_file) {
        // Write some data to the file
        const char* test_data = "Hello from DingOS! This is a test file created by the ext2 filesystem.";
        test_file->write_all(0, strlen_ext(test_data), (char*)test_data);
        
        printf("File '%s' created and written successfully\n", test_filename);
        
        // Close the file
        delete test_file;
        
        // Now read it back
        test_file = find_in_directory(fs->root, test_filename);
        if (test_file) {
            char buffer[256];
            int64_t bytes_read = test_file->read_all(0, sizeof(buffer) - 1, buffer);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Read from test file (%lld bytes):\n%s\n", bytes_read, buffer);
            } else {
                printf("Error reading file\n");
            }
            
            delete test_file;
        } else {
            printf("Error: Could not find newly created file\n");
        }
    } else {
        printf("Error: Failed to create test file\n");
    }
    
    // Show the updated directory listing
    printf("Updated root directory contents:\n");
    list_directory(fs->root);
    
    // Clean up
    delete fs;
    delete adapter;
}

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