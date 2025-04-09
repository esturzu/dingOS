#ifndef _BFS_H_
#define _BFS_H_

#include "physmem.h"
#include "printf.h"
#include "stdint.h"
#include "sd.h"  

// Filesystem constants
#define BLOCK_SIZE 1024
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAGIC_NUMBER 0xDEADBEEF  // Magic number for FS verification

// Superblock structure (keeps metadata)
struct super_block_bfs {
    uint32_t num_iNodes;
    uint32_t num_Blocks;
    char pad0[16];
    uint32_t block_size;
    char pad1[4];
    uint32_t num_blocks_pergroup;
    char pad2[4];
    uint32_t num_iNode_pergroup;
    char pad3[12];
    uint16_t magic;
    char pad4[30];
    uint16_t iNode_size;
} __attribute__((packed));

struct group_descriptor_bfs {
    uint32_t bit_map_block_address;
    uint32_t bit_map_iNode_address;
    uint32_t startingBlockAddress;
    uint16_t num_unallocated_blocks;
    uint16_t num_unallocated_iNodes;
    uint16_t num_unallocated_directories;
    char pad0[14];
}__attribute__((packed));

struct ext2_dir_entry_bfs {
    uint32_t iNodeNum;
    uint16_t size_entry;
    uint8_t name_length;
    uint8_t type_indicator;
    char* name_characters;
};

struct Inode_bfs {
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

struct FileEntry_bfs {
    char name[MAX_FILENAME];
    uint32_t inode_index;
};

// Public API
void strncpy_bfs(char* dest, const char* src, uint32_t n);
void fs_init_bfs();
int fs_create_bfs(const char* name, uint32_t size);
int fs_read_bfs(const char* name, char* buffer);
int fs_write_bfs(const char* name, const char* data, uint32_t size);
void fs_list_bfs();

#endif  // _BFS_H_
