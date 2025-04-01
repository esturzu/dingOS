#include "ext2.h"

// Initialize the global variables
super_block* supa = nullptr;
BGDT* bgdt = nullptr;
uint32_t SB_offset = 1024;
uint32_t BGDT_index = 0;


// Simple function to debug dump a block
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

// Function to initialize ext2 filesystem
void init_ext2() {
    // Initialize the SD card
    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD card initialization failed!\n");
        return;
    }
    
    // Create an SD adapter with block size 1024 (initial value, will adjust after reading superblock)
    SDAdapter* adapter = new SDAdapter(1024);
    
    // Read the superblock
    supa = new super_block();
    adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
    
    // Verify the magic number (TODO)
    
    
    // Update the adapter block size based on superblock
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
    
    adapter->read_all(BGDT_index * fs_block_size, sizeof(BGDT), (char*)bgdt);
    
    // Print BGDT info in the exact format requested
    printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
           bgdt->bit_map_block_address,
           bgdt->bit_map_iNode_address,
           bgdt->startingBlockAddress,
           bgdt->num_unallocated_blocks,
           bgdt->num_unallocated_directories,
           bgdt->num_unallocated_iNodes);
    
    // Create an ext2 filesystem instance
    Ext2* fs = new Ext2(adapter);
    
    // Print root directory information
    if (fs->root && fs->root->is_dir()) {
        printf("Root directory found (inode %u)\n", fs->root->number);
    } else {
        printf("Error: Root is not a directory or not found\n");
    }
}