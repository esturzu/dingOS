#include "ext2.h"

/*
 * EXT2 FILESYSTEM IMPLEMENTATION
 *
 * This file implements the ext2 filesystem operations defined in ext2.h.
 * It provides functionality to:
 * - Initialize the filesystem
 * - List directory contents
 * - Find files and directories
 * - Read file contents
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
    
}