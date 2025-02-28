#include "bfs.h"
#include "physmem.h"
#include "sd.h"

// Global file system state
Superblock superblock;
FileEntry file_table[MAX_FILES];
Inode inode_table[MAX_FILES];

// Simple string comparison function, will go to our libk eventually
bool streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}
// copies strings, duhhhh
void strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}


// Initialize the filesystem
void fs_init() {
    printf("Initializing Minimal Filesystem...\n");

    // if (SD::init() != SD::SUCCESS) {
    //     printf("ERROR: SD card initialization failed!\n");
    //     return;
    // }

    // Initialize superblock
    superblock.magic = MAGIC_NUMBER;
    superblock.total_blocks = 1024;
    superblock.free_blocks = superblock.total_blocks - 2;
    superblock.block_size = BLOCK_SIZE;

    // Initialize file table
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].name[0] = '\0';
        inode_table[i].size = 0;
    }

    printf("Filesystem initialized using SD card!\n");
}

// Create a new file
int fs_create(const char* name, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] == '\0') {
            strncpy(file_table[i].name, name, MAX_FILENAME);
            file_table[i].name[MAX_FILENAME - 1] = '\0';  // Ensure null termination
            file_table[i].inode_index = i;
            inode_table[i].size = size;
            return 0;
        }
    }
    return -1;
}

// Read a file (copies data into `buffer`)
int fs_read(const char* name, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            return SD::read(file_table[i].inode_index, 1, (uint8_t*)buffer);

        }
    }
    return -1;
}

// Write data to a file
int fs_write(const char* name, const char* data, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            debug_printf("fs_write: Writing to block %d\n", file_table[i].inode_index);
            int result = SD::write(file_table[i].inode_index, 1, (uint8_t*)data);
            debug_printf("fs_write: SD write returned %d\n", result);


            if (result < 0) {  // Keep this for actual errors
                debug_printf("fs_write: Error writing to SD!\n");
                return -1;
            } else if (result == SD::BLOCKSIZE) {  // Allow block-sized writes
                debug_printf("fs_write: Successfully wrote one full block!\n");
                return 0;
            } else {
                debug_printf("fs_write: Unexpected write size %d!\n", result);
                return -1;
            }

        }
    }
    return -1;
}

// List all files
void fs_list() {
    printf("Filesystem contents:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] != '\0') {
            printf(" - %s (%d bytes)\n", file_table[i].name, inode_table[i].size);
        }
    }
}
