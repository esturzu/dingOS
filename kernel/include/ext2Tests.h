#ifndef EXT2_TESTS_H
#define EXT2_TESTS_H

#include "ext2.h"
#include "testFramework.h"

void ext2Tests() {
    initTests("Ext2 Filesystem Tests");

    debug_printf("Initializing SD Card and Filesystem...\n");
    
    // Initialize SD Card and Filesystem
    SDAdapter* adapter = new SDAdapter(1024);
    Ext2* fs = new Ext2(adapter);

    debug_printf("Filesystem Initialized. Block Size: %u\n", fs->get_block_size());

    // Detailed Block Bitmap Verification
    debug_printf("\n--- Initial Block Bitmap State ---\n");
    
    // Read the block bitmap
    uint8_t* bitmap = new uint8_t[1024 << fs->get_block_size()];
    adapter->read_block(bgdt->bit_map_block_address, (char*)bitmap);

    // Bitmap analysis
    uint32_t total_blocks = supa->num_Blocks;
    uint32_t used_blocks = 0;
    uint32_t first_used_block = 0;
    uint32_t last_used_block = 0;

    debug_printf("Block Bitmap Location: %u\n", bgdt->bit_map_block_address);
    debug_printf("Total Blocks: %u\n", total_blocks);
    debug_printf("Reported Free Blocks: %u\n", bgdt->num_unallocated_blocks);

    // Detailed bitmap scan
    for (uint32_t i = 0; i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if (bitmap[byte_idx] & (1 << bit_idx)) {
            used_blocks++;
            if (first_used_block == 0) first_used_block = i;
            last_used_block = i;
        }
    }

    debug_printf("Used Blocks: %u\n", used_blocks);
    debug_printf("Calculated Free Blocks: %u\n", total_blocks - used_blocks);
    debug_printf("First Used Block: %u\n", first_used_block);
    debug_printf("Last Used Block: %u\n", last_used_block);

    // Perform file operations to track block allocation
    debug_printf("\n--- Performing File Operations ---\n");
    
    // Create a file to force block allocation
    Node* test_file = create_file(fs->root, "allocation_test.txt");
    if (test_file) {
        const char* test_content = "Testing block allocation mechanisms in DingOS ext2 filesystem.";
        int64_t write_result = test_file->write_all(0, strlen_ext(test_content), (char*)test_content);
        
        debug_printf("Wrote %lld bytes to test file\n", write_result);
        delete test_file;
    }

    // Re-read bitmap after operations
    adapter->read_block(bgdt->bit_map_block_address, (char*)bitmap);
    
    used_blocks = 0;
    for (uint32_t i = 0; i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if (bitmap[byte_idx] & (1 << bit_idx)) {
            used_blocks++;
        }
    }
    diagnose_block_bitmap(adapter);

    debug_printf("\n--- Post-Operation Block Bitmap State ---\n");
    debug_printf("Used Blocks: %u\n", used_blocks);
    debug_printf("Calculated Free Blocks: %u\n", total_blocks - used_blocks);
    debug_printf("BGDT Reported Free Blocks: %u\n", bgdt->num_unallocated_blocks);

    delete[] bitmap;
    delete fs;
    delete adapter;
}

#endif // EXT2_TESTS_H