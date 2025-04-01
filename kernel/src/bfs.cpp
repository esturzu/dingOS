#include "bfs.h"
#include "physmem.h"
#include "sd.h"

// Global file system state
// This is where we keep track of everything for the filesystem. 
// The superblock holds overall metadata about the filesystem.
// The file_table keeps track of which files exist and their names.
// The inode_table keeps track of file sizes and, eventually, their actual data locations (for now, it's just a size mapping).
super_block superblock;
FileEntry file_table[MAX_FILES];
Inode inode_table[MAX_FILES];

// Simple string comparison function, will go to our libk eventually
// This is because I don't want to deal with the standard library stuff right now.
// It does what you'd expect—compares two C-style strings for equality.
bool streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;  // If any character mismatches, return false immediately.
        a++; b++;  // Move to the next character in both strings.
    }
    if(*a == *b) {
        printf("in streq and it's true");
    }
    return *a == *b;  // Return true if both strings end at the same time.
}

void zero_memory(uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
}


// copies strings, duhhhh
// The simplest string copy function possible, without using `memcpy` (which I don’t have).
// It copies characters from `src` into `dest` up to `n - 1` characters and then manually null-terminates the result.
void strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];  // Copy character-by-character.
    }
    dest[i] = '\0';  // Ensure null termination, just in case the source string was too long.
}

void dump_blocks(uint32_t start_block, uint32_t count) {
    uint8_t block[BLOCK_SIZE];

    for (uint32_t b = 0; b < count; b++) {
        uint32_t blknum = start_block + b;
        if (SD::read(blknum, 1, block) != BLOCK_SIZE) {
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


void fs_init() {
    printf("Initializing Minimal Filesystem from SD...\n");
    // Initialize SD card
    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD card initialization failed!\n");
        return;
    }

    // Read block 1 (ext2 superblock starts at BLOCK_SIZE bytes)
    uint8_t block[BLOCK_SIZE];
    if (SD::read(1, 1, block) != BLOCK_SIZE) {
        printf("ERROR: Failed to read block 1 from SD.\n");
        return;
    }

    // Interpret as ext2 superblock (your struct must be packed!)
    super_block* sb = (super_block*)block;

    // Check ext2 magic number
    if (sb->magic != 0xEF53) {
        printf("ERROR: Invalid ext2 superblock. Got magic = 0x%x\n", sb->magic);
        return;
    }

    // Store the block into your global if needed
    superblock = *sb;

    printf("EXT2 superblock loaded!\n");
    printf("Blocks: %u | Inodes: %u | Block size = %u | Inodes per group = %u\n",
           superblock.num_Blocks,
           superblock.num_iNodes,
           1024 << superblock.block_size,  // real block size
           superblock.num_iNode_pergroup);
  
    uint8_t gd_block[1024 << superblock.block_size];
    if (SD::read(2, 1, gd_block) != 1024 << superblock.block_size) {
        printf("ERROR: Failed to read group descriptor\n");
        return;
    }

    // Parse the group descriptor
    group_descriptor* gd = (group_descriptor*)gd_block;
    printf("BGDT superblock loaded!\n");
    printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
           gd->bit_map_block_address,
           gd->bit_map_iNode_address,
           gd->startingBlockAddress,
           gd->num_unallocated_blocks,
           gd->num_unallocated_directories,
           gd->num_unallocated_iNodes
        );
    
    
    printf("Filesystem initialized using SD card!\n");
    dump_blocks(6, 1);

}


// Create a new file
// This function tries to create a new file with the given name and size.
// It searches for an empty spot in `file_table`, fills in the name, and assigns an inode.
int fs_create(const char* name, uint32_t size) {
    // for (int i = 0; i < MAX_FILES; i++) {
    //     if (file_table[i].name[0] == '\0') {  // Found an empty slot.
    //         strncpy(file_table[i].name, name, MAX_FILENAME);  // Copy the filename.
    //         file_table[i].name[MAX_FILENAME - 1] = '\0';  // Ensure null termination, took me a while to realize this *face palm*
    //         file_table[i].inode_index = i;  // Using the array index as the inode index (not great long-term, but whatever).
    //         inode_table[i].size = size;  // Set the size of the new file.
    //         return 0;  // Success!
    //     }
    // }
    return -1;  // No available space for a new file.
}

// Read a file (copies data into `buffer`)
// This function looks up a file by name and reads its contents into `buffer`.
// Right now, it assumes the entire file fits into a single block, which will **definitely** need to change later.
int fs_read(const char* name, char* buffer) {
    // Find the file with the correct name, right now i am guessing i cannot have two files with the same exact name...
    // This is just a simple linear search in `file_table`, which is obviously inefficient if there are a lot of files.
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            // Fill the read buffer with data from disk, count is 1 because we are doing one block for now, to be expanded
            return SD::read(file_table[i].inode_index, 1, (uint8_t*)buffer);
        }
    }
    return -1;  // File not found.
}

// Write data to a file
// This function looks up a file by name and writes `size` bytes from `data` into it.
// Again, for now, it's restricted to writing a single block at a time.
int fs_write(const char* name, const char* data, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        // again find file
        if (streq(name, file_table[i].name)) {
            debug_printf("fs_write: Writing to block %d\n", file_table[i].inode_index);
            int result = SD::write(file_table[i].inode_index, 1, (uint8_t*)data);
            debug_printf("fs_write: SD write returned %d\n", result);

            if (result < 0) { 
                debug_printf("fs_write: Error writing to SD!\n");
                return -1;
            } else if (result == SD::BLOCKSIZE) {  
                // again, can only write one block
                // Eventually, we need to support files that span multiple blocks, but this is fine for now.
                debug_printf("fs_write: Successfully wrote one full block!\n");
                return 0;
            } else {
                // If the write operation returned something unexpected, print a warning.
                debug_printf("fs_write: Unexpected write size %d!\n", result);
                return -1;
            }
        }
    }
    return -1;  // File not found.
}

// List all files
// Right now, this is just a simple iteration through the `file_table`.
// Eventually, this needs to go down the recursive ext2-style pointer structure (so, directories and metadata blocks).
// Right now, there **are no directories**, just a flat list of files.
void fs_list() {
    printf("Filesystem contents:\n");
    // for (int i = 0; i < MAX_FILES; i++) {
    //     if (file_table[i].name[0] != '\0') {  // If a file exists in this slot, print it.
    //         printf(" - %s (%d bytes)\n", file_table[i].name, inode_table[i].size);
    //     }
    // }
}
