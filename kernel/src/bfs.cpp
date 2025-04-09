#include "bfs.h"
#include "physmem.h"
#include "sd.h"
#include "block_io.h"

// Global file system state
super_block_bfs superblock_bfs;
FileEntry_bfs file_table_bfs[MAX_FILES];
Inode_bfs inode_table_bfs[MAX_FILES];

bool streq_bfs(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    if (*a == *b) {
        printf("in streq and it's true");
    }
    return *a == *b;
}

void zero_memory_bfs(uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
}

void strncpy_bfs(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void dump_blocks_bfs(uint32_t start_block, uint32_t count) {
    uint8_t block[BLOCK_SIZE];

    for (uint32_t b = 0; b < count; b++) {
        uint32_t blknum = start_block + b;
        if (SD::read(blknum, 2, block) != BLOCK_SIZE) {
            printf("Failed to read block %u\n", blknum);
            continue;
        }

        printf("Block %u:\n", blknum);
        for (int i = 0; i < BLOCK_SIZE; i++) {
            printf("%02x ", block[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }
}

void fs_init_bfs() {
    printf("Initializing Minimal Filesystem from SD...\n");
    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD card initialization failed!\n");
        return;
    }

    uint8_t block[BLOCK_SIZE];
    if (SD::read(2, 2, block) != BLOCK_SIZE) {
        printf("ERROR: Failed to read block 1 from SD.\n");
        return;
    }

    super_block_bfs* sb = (super_block_bfs*)block;

    if (sb->magic != 0xEF53) {
        printf("ERROR: Invalid ext2 superblock. Got magic = 0x%x\n", sb->magic);
        return;
    }

    superblock_bfs = *sb;

    printf("EXT2 superblock loaded!\n");
    printf("Blocks: %u | Inodes: %u | Block size = %u | Inodes per group = %u\n",
           superblock_bfs.num_Blocks,
           superblock_bfs.num_iNodes,
           1024 << superblock_bfs.block_size,
           superblock_bfs.num_iNode_pergroup);

    uint8_t gd_block[1024 << superblock_bfs.block_size];
    if (SD::read(4, 2, gd_block) != 1024 << superblock_bfs.block_size) {
        printf("ERROR: Failed to read group descriptor\n");
        return;
    }

    group_descriptor_bfs* gd = (group_descriptor_bfs*)gd_block;
    printf("BGDT superblock loaded!\n");
    printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
           gd->bit_map_block_address,
           gd->bit_map_iNode_address,
           gd->startingBlockAddress,
           gd->num_unallocated_blocks,
           gd->num_unallocated_directories,
           gd->num_unallocated_iNodes);

    printf("Filesystem initialized using SD card!\n");
    dump_blocks_bfs(6, 1);
}

int fs_create_bfs(const char* name, uint32_t size) {
    return -1;
}

int fs_read_bfs(const char* name, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq_bfs(name, file_table_bfs[i].name)) {
            return SD::read(file_table_bfs[i].inode_index, 1, (uint8_t*)buffer);
        }
    }
    return -1;
}

int fs_write_bfs(const char* name, const char* data, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq_bfs(name, file_table_bfs[i].name)) {
            debug_printf("fs_write: Writing to block %d\n", file_table_bfs[i].inode_index);
            int result = SD::write(file_table_bfs[i].inode_index, 1, (uint8_t*)data);
            debug_printf("fs_write: SD write returned %d\n", result);

            if (result < 0) {
                debug_printf("fs_write: Error writing to SD!\n");
                return -1;
            } else if (result == SD::BLOCKSIZE) {
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

void fs_list_bfs() {
    printf("Filesystem contents:\n");
}
