#ifndef _BFS_H_
#define _BFS_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "stdint.h"

// Filesystem constants
#define BLOCK_SIZE 4096
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAGIC_NUMBER 0xDEADBEEF  // To identify our FS

// Superblock structure (keeps metadata)
struct Superblock {
    uint32_t magic;         // Magic number for FS identification
    uint32_t total_blocks;  // Total blocks in filesystem
    uint32_t free_blocks;   // Number of free blocks
    uint32_t block_size;    // Usually 4KB
};

// Inode structure (stores file metadata)
struct Inode {
    uint32_t size;                  // File size in bytes
    uint32_t direct_blocks[12];      // Directly point to data blocks
};

// File table entry (maps filenames to inodes)
struct FileEntry {
    char name[MAX_FILENAME];  // Filename (fixed size)
    uint32_t inode_index;     // Index into inode table
};

// Public API
void fs_init();                          // Initialize filesystem
int fs_create(const char* name, uint32_t size);  // Create file
int fs_read(const char* name, char* buffer);     // Read file
int fs_write(const char* name, const char* data, uint32_t size); // Write file
void fs_list();                            // List all files

#endif  // _MFS_H_
