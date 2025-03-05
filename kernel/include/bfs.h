#ifndef _BFS_H_
#define _BFS_H_

#include "physmem.h"
#include "printf.h"
#include "stdint.h"
#include "sd.h"  // Now directly using SD instead of BlockIO

// Filesystem constants
#define BLOCK_SIZE 4096
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAGIC_NUMBER 0xDEADBEEF  // Magic number for FS verification

// Superblock structure (keeps metadata)
struct Superblock {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t block_size;
};

// Inode structure (stores file metadata)
struct Inode {
    uint32_t size;
    uint32_t direct_blocks[12];
};

// File table entry (maps filenames to inodes)
struct FileEntry {
    char name[MAX_FILENAME];
    uint32_t inode_index;
};

// Public API
void strncpy(char* dest, const char* src, uint32_t n);
void fs_init();  // Initialize the filesystem
int fs_create(const char* name, uint32_t size);  // Create a file
int fs_read(const char* name, char* buffer);  // Read a file
int fs_write(const char* name, const char* data, uint32_t size);  // Write to a file
void fs_list();  // List all files

#endif  // _BFS_H_
