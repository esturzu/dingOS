#include "bfs.h"
#include "physmem.h"

// Global file system state
Superblock superblock;
FileEntry file_table[MAX_FILES];
Inode inode_table[MAX_FILES];

// Block storage in RAM (temporary until we add SD support)
char* storage;

// random helper functions instead of stuff from gheith's libk, just for now
bool streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

// Initialize the filesystem
void fs_init() {
    printf("Initializing Minimal Filesystem...\n");

    // Allocate RAM storage for filesystem (equivalent to a RAM disk)

    // storage = (char*) PhysMem::allocate_frame();  
    // if (!storage) {
    //     printf("ERROR: Could not allocate storage for filesystem.\n");
    //     return;
    // }

    // forced mem address
    storage = (char*) 0x200A0000;  
    printf("TEST: Using forced storage address: 0x%X\n", storage);


    // Initialize superblock
    superblock.magic = MAGIC_NUMBER;
    superblock.total_blocks = 1024;  // Hardcoded for now
    superblock.free_blocks = superblock.total_blocks - 2; // Reserve superblock & file table
    superblock.block_size = BLOCK_SIZE;

    // Clear file table and inodes
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].name[0] = '\0';  // Mark as empty
        inode_table[i].size = 0;
    }

    printf("Filesystem initialized.\n");
}

// Create a new file
int fs_create(const char* name, uint32_t size) {
    printf("Creating file: %s, size: %d bytes\n", name, size);

    // Find an empty slot in file table
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] == '\0') {  // Empty entry found
            // Store filename
            int j;
            for (j = 0; j < MAX_FILENAME - 1 && name[j] != '\0'; j++) {
                file_table[i].name[j] = name[j];
            }
            file_table[i].name[j] = '\0';

            // Assign an inode
            file_table[i].inode_index = i;
            inode_table[i].size = size;

            printf("File '%s' created with inode %d.\n", name, i);
            return 0;
        }
    }

    printf("ERROR: File table full.\n");
    return -1;
}

// Read a file (copies data into `buffer`)
int fs_read(const char* name, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            int size = inode_table[i].size;
            char* file_data = storage + (i * BLOCK_SIZE);
            for (int j = 0; j < size; j++) {
                buffer[j] = file_data[j];
            }
            printf("Read %d bytes from '%s'.\n", size, name);
            return size;
        }
    }
    printf("ERROR: File not found.\n");
    return -1;
}

// Write data to a file
int fs_write(const char* name, const char* data, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            inode_table[i].size = size;
            char* file_data = storage + (i * BLOCK_SIZE);
            for (int j = 0; j < size; j++) {
                file_data[j] = data[j];
            }
            printf("Wrote %d bytes to '%s'.\n", size, name);
            return 0;
        }
    }
    printf("ERROR: File not found.\n");
    return -1;
}

// List all files
void fs_list() {
    debug_printf("Filesystem contents:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] != '\0') {
            printf(" - %s (%d bytes)\n", file_table[i].name, inode_table[i].size);
        }
    }
}
