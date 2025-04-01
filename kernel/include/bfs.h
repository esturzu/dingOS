#ifndef _BFS_H_
#define _BFS_H_

#include "physmem.h"
#include "printf.h"
#include "stdint.h"
#include "sd.h"  // Now directly using SD instead of BlockIO

// Filesystem constants
#define BLOCK_SIZE 1024
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAGIC_NUMBER 0xDEADBEEF  // Magic number for FS verification

// Superblock structure (keeps metadata)
struct super_block {
    uint32_t num_iNodes;             // 0x00
    uint32_t num_Blocks;             // 0x04
    char pad0[16];                   // 0x08–0x17 (reserved blocks, etc.)
    uint32_t block_size;             // 0x18
    char pad1[4];                    // 0x1C
    uint32_t num_blocks_pergroup;   // 0x20
    char pad2[4];                    // 0x24
    uint32_t num_iNode_pergroup;    // 0x28
    char pad3[12];                   // 0x2C–0x37
    uint16_t magic;                 // 0x38 ← EXT2 magic number (0xEF53)
    char pad4[30];                   // 0x3A–0x57 (whatever follows)
    uint16_t iNode_size;             // 0x58
} __attribute__((packed));

struct group_descriptor {
    uint32_t bit_map_block_address;
    uint32_t bit_map_iNode_address;
    uint32_t startingBlockAddress;
    uint16_t num_unallocated_blocks;
    uint16_t num_unallocated_iNodes;
    uint16_t num_unallocated_directories;
    char pad0[14];
}__attribute__((packed));


struct ext2_dir_entry {
    uint32_t iNodeNum;
    uint16_t size_entry;
    uint8_t name_length;
    uint8_t type_indicator;
    char* name_characters;
};


struct Inode {
    uint16_t types_plus_perm;
    char pad0[2];
    uint32_t size_of_iNode;
    char pad1[18];
    uint16_t num_Hard_Links;
    char pad2[12];
    uint32_t directLinked[12];
    uint32_t singleIndirect;
    uint32_t doubleIndirect;
    uint32_t tripleIndirect;
    char pad3[28];
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
