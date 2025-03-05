#include "bfs.h"
#include "physmem.h"
#include "block_io.h"  // Changed! Use BlockIO wrapper! Changed!

// Global file system state
// This is where we keep track of everything for the filesystem. 
// The superblock holds overall metadata about the filesystem.
// The file_table keeps track of which files exist and their names.
// The inode_table keeps track of file sizes and, eventually, their actual data locations (for now, it's just a size mapping).
Superblock superblock;
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
    return *a == *b;  // Return true if both strings end at the same time.
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

// Initialize the filesystem
void fs_init() {
    debug_printf("Initializing Minimal Filesystem...\n");

    // Initialize superblock
    // This will eventually be read from the disk when mounting a real ext2/ext3 filesystem,
    // but for now, I’m just hardcoding some values to get things rolling.
    // The total number of blocks is arbitrarily set to 1024.
    // The "free_blocks" count is total_blocks - 2, because I'm assuming a few blocks are reserved.
    superblock.magic = MAGIC_NUMBER;
    superblock.total_blocks = 1024;
    superblock.free_blocks = superblock.total_blocks - 2;
    superblock.block_size = BLOCK_SIZE;  // Fixed block size, currently hardcoded.

    // Initialize file table, will be simplified when new is better i think...
    // Right now, we're treating this as a flat array. 
    // Later, this should be dynamically allocated, probably with a better memory management strategy.
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].name[0] = '\0';  // Mark as empty.
        inode_table[i].size = 0;  // Default file size is 0 (makes sense).
    }

    debug_printf("Filesystem initialized using SD card!\n");
}

// Create a new file
// This function tries to create a new file with the given name and size.
// It searches for an empty spot in `file_table`, fills in the name, and assigns an inode.
int fs_create(const char* name, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] == '\0') {  // Found an empty slot.
            strncpy(file_table[i].name, name, MAX_FILENAME);  // Copy the filename.
            file_table[i].name[MAX_FILENAME - 1] = '\0';  // Ensure null termination, took me a while to realize this *face palm*
            file_table[i].inode_index = i;  // Using the array index as the inode index (not great long-term, but whatever).
            inode_table[i].size = size;  // Set the size of the new file.
            return 0;  // Success!
        }
    }
    return -1;  // No available space for a new file.
}

// Read a file (copies data into `buffer`)
// This function looks up a file by name and reads its contents into `buffer`.
// Now supports multi-block reads.
int fs_read(const char* name, char* buffer, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            uint32_t num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            debug_printf("fs_read: Reading %d blocks from %s (inode %d)\n", num_blocks, name, file_table[i].inode_index);

            // Changed! Read multiple blocks properly! Changed!
            BlockIO::read(file_table[i].inode_index, num_blocks, buffer);
            
            debug_printf("fs_read: Successfully read %d bytes from %s\n", size, name);
            return size;  // Return the requested size (assuming full read success)
            // Changed!
        }
    }
    debug_printf("fs_read: File %s not found!\n", name);
    return -1;  // File not found.
}

// Write data to a file
// This function looks up a file by name and writes `size` bytes from `data` into it.
// Supports multi-block writes now.
int fs_write(const char* name, const char* data, uint32_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (streq(name, file_table[i].name)) {
            uint32_t num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            debug_printf("fs_write: Writing %d blocks to %s (inode %d)\n", num_blocks, name, file_table[i].inode_index);

            // Changed! Write multiple blocks properly! Changed!
            BlockIO::write(file_table[i].inode_index, num_blocks, data);
            
            debug_printf("fs_write: Successfully wrote %d bytes to %s\n", size, name);
            return size;  // Return the requested size (assuming full write success)
            // Changed!
        }
    }
    debug_printf("fs_write: File %s not found!\n", name);
    return -1;  // File not found.
}

// List all files
// Right now, this is just a simple iteration through the `file_table`.
// Eventually, this needs to go down the recursive ext2-style pointer structure (so, directories and metadata blocks).
// Right now, there **are no directories**, just a flat list of files.
void fs_list() {
    debug_printf("Filesystem contents:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].name[0] != '\0') {  // If a file exists in this slot, print it.
            debug_printf(" - %s (%d bytes)\n", file_table[i].name, inode_table[i].size);
        }
    }
}
