#ifndef _EXT2_H_
#define _EXT2_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"

/*
 * EXT2 Filesystem Implementation
 * 
 * This implementation provides read-only access to EXT2 filesystems on an SD card.
 * It includes functionality to:
 * - Read the superblock and block group descriptor table
 * - Navigate the directory structure
 * - Read files
 * 
 * Design overview:
 * - SDAdapter: Bridges between BlockIO and SD card operations
 * - Node: Represents files, directories, and symlinks
 * - Ext2: Main filesystem class that mounts and manages the filesystem
 */

// Constants for the ext2 filesystem
#define EXT2_SUPER_MAGIC 0xEF53    // Magic signature for ext2 filesystems
#define EXT2_SB_OFFSET 1024        // Superblock is located 1KB from start of volume

/*
 * FILESYSTEM STRUCTURES
 * 
 * These structures match the on-disk layout of ext2 filesystem structures.
 * All fields are kept in their original order and size to match the ext2 specification.
 */

// Superblock structure - contains filesystem metadata
struct super_block {
    uint32_t num_iNodes;           // Total number of inodes
    uint32_t num_Blocks;           // Total number of blocks
    char pad0[16];                 // Reserved/unused fields
    uint32_t block_size;           // Block size as a power of 2 (0=1K, 1=2K, etc.)
    char pad1[4];                  // Reserved/unused fields
    uint32_t num_blocks_pergroup;  // Number of blocks per block group
    char pad2[4];                  // Reserved/unused fields
    uint32_t num_iNode_pergroup;   // Number of inodes per block group
    char pad3[44];                 // Reserved/unused fields
    uint16_t iNode_size;           // Size of each inode structure
};

// iNode structure - represents a file, directory, or symlink
struct iNode {
    uint16_t types_plus_perm;      // Type (file/dir/symlink) and permissions
    char pad0[2];                  // Reserved/unused fields
    uint32_t size_of_iNode;        // Size in bytes
    char pad1[18];                 // Reserved/unused fields
    uint16_t num_Hard_Links;       // Number of hard links to this inode
    char pad2[12];                 // Reserved/unused fields
    uint32_t directLinked[12];     // Direct block pointers (first 12 blocks)
    uint32_t singleIndirect;       // Single indirect block pointer
    uint32_t doubleIndirect;       // Double indirect block pointer
    uint32_t tripleIndirect;       // Triple indirect block pointer
    char pad3[28];                 // Reserved/unused fields
};

// Block Group Descriptor Table entry - describes a block group
struct BGDT {
    uint32_t bit_map_block_address;    // Block containing the block bitmap
    uint32_t bit_map_iNode_address;    // Block containing the inode bitmap
    uint32_t startingBlockAddress;     // Block containing the first inode table block
    uint16_t num_unallocated_blocks;   // Number of unallocated blocks in group
    uint16_t num_unallocated_iNodes;   // Number of unallocated inodes in group
    uint16_t num_unallocated_directories; // Number of directories in group
    char pad0[14];                    // Reserved/unused fields
};

// Directory entry structure - links names to inodes
struct dir_entry {
    uint32_t iNodeNum;             // Inode number
    uint16_t size_entry;           // Total size of this entry (including name)
    uint8_t name_length;           // Length of the name
    uint8_t type_indicator;        // Type of file (optional in some ext2 versions)
    char name_characters[256];     // Name (variable length, null-terminated for convenience)
};

// Global variables accessible throughout the filesystem
extern super_block* supa;          // Pointer to the mounted filesystem's superblock
extern BGDT* bgdt;                 // Pointer to the first block group descriptor
extern uint32_t SB_offset;         // Offset to the superblock (1024 bytes)
extern uint32_t BGDT_index;        // Block index of the BGDT

// Forward declarations
class Node;

/*
 * SD ADAPTER CLASS
 * 
 * This class bridges between the BlockIO interface and the SD card driver.
 * It handles the mapping between filesystem blocks and SD card sectors.
 */
class SDAdapter : public BlockIO {
public:
    // Constructor takes the block size (can be different from SD sector size)
    SDAdapter(uint32_t block_size) : BlockIO(block_size) {}
    
    // Destructor
    virtual ~SDAdapter() {}
    
    // Returns a large enough size to represent the entire device
    uint32_t size_in_bytes() override {
        return 0xFFFFFFFF; // Large value representing the whole SD card
    }
    
    // Read a block from the SD card
    void read_block(uint32_t block_number, char* buffer) override {
        // Calculate how many SD sectors make up one filesystem block
        uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
        // Calculate the starting sector
        uint32_t sd_start = block_number * sd_block_count;
        // Read the sectors
        uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)buffer);
        // Check if read was successful
        if (bytes != block_size) {
            printf("SDAdapter::read_block: Failed to read block %u\n", block_number);
        }
    }
    
    // Write a block to the SD card (stub implementation)
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override {
        // Not implemented for read-only filesystem
        printf("SDAdapter::write_block: Not implemented\n");
    }
};

/*
 * NODE CLASS
 * 
 * This class represents an inode in the filesystem (file, directory, or symlink).
 * It inherits from BlockIO to provide block-level access to the node's data.
 */
class Node : public BlockIO {
public:
    const uint32_t number;         // Inode number
    iNode* node;                   // Pointer to the inode data
    uint32_t block_group;          // Block group containing this inode
    uint32_t index;                // Index of this inode within its block group
    uint32_t containing_block;     // Block containing this inode
    uint32_t BGDT_index;           // Block index of the BGDT
    SDAdapter* sd_adapter;         // Pointer to the SD adapter
    bool have_entry_before;        // Cache flag for directory entry count
    uint32_t entry_result;         // Cached directory entry count
    char* oldSym;                  // Cache for symlink content

    // Constructor loads an inode from disk
    Node(uint32_t block_size, uint32_t number, SDAdapter* adapter) 
        : BlockIO(block_size), number(number), node(nullptr), 
          sd_adapter(adapter), have_entry_before(false), entry_result(0), oldSym(nullptr) {
        
        // Allocate and initialize the inode
        node = new iNode();
        
        // Calculate which block group contains this inode
        block_group = (number - 1) / supa->num_iNode_pergroup;
        // Calculate index within the block group
        index = (number - 1) % supa->num_iNode_pergroup;
        
        // Set BGDT index based on block size
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;
        } else {
            BGDT_index = 1;
        }
        
        // Read the inode data from disk
        uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                               index * supa->iNode_size;
        sd_adapter->read_all(inode_offset, sizeof(iNode), (char*)node);
    }
    
    // Destructor cleans up allocated memory
    virtual ~Node() {
        delete node;
        if (oldSym) delete[] oldSym;
    }
    
    // Returns the size of this node in bytes
    uint32_t size_in_bytes() override {
        if (is_dir()) {
            // For directories, we'll count entries later
            return node->size_of_iNode;
        } else if (is_file()) {
            // For files, return the file size
            return node->size_of_iNode;
        } else if (is_symlink()) {
            // For symlinks, return the link name length
            return node->size_of_iNode;
        }
        return 0;
    }
    
    // Read a block from this node
    void read_block(uint32_t block_number, char* buffer) override {
        // Calculate how many pointers fit in one block
        uint32_t N = (1024 << supa->block_size) / 4;
        
        if (block_number < 12) {
            // Direct block - directly referenced by the inode
            sd_adapter->read_all(node->directLinked[block_number] * (1024 << supa->block_size), 
                             (1024 << supa->block_size), buffer);
        } else if (block_number < (12 + N)) {
            // Single indirect block - referenced through a single indirection
            printf("Node::read_block: Single indirect blocks not implemented\n");
        } else if (block_number < (12 + N + (N * N))) {
            // Double indirect block - referenced through double indirection
            printf("Node::read_block: Double indirect blocks not implemented\n");
        } else {
            // Triple indirect block - referenced through triple indirection
            printf("Node::read_block: Triple indirect blocks not implemented\n");
        }
    }
    
    // Write a block to this node (stub implementation)
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override {
        // Not implemented for read-only filesystem
        printf("Node::write_block: Not implemented\n");
    }
    
    // Extract the file type from the mode/permissions field
    uint32_t get_type() {
        uint32_t type = (node->types_plus_perm >> 12) << 12;
        return type;
    }
    
    // Check if this node is a directory
    bool is_dir() {
        uint32_t dir_code = 16384;  // 0x4000
        return (get_type() == dir_code);
    }
    
    // Check if this node is a regular file
    bool is_file() {
        uint32_t file_code = 32768;  // 0x8000
        return (get_type() == file_code);
    }
    
    // Check if this node is a symbolic link
    bool is_symlink() {
        uint32_t sym_link = 40960;  // 0xA000
        return (get_type() == sym_link);
    }
};

/*
 * EXT2 FILESYSTEM CLASS
 * 
 * This class represents a mounted ext2 filesystem.
 * It reads the superblock and block group descriptors,
 * and provides access to the root directory.
 */
class Ext2 {
public:
    Node* root;  // Root directory node (always inode 2 in ext2)
    
    // Constructor mounts the filesystem
    Ext2(SDAdapter* adapter) {
        // Read the superblock
        supa = new super_block();
        adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
        
        // Verify the filesystem magic number (TODO)
        
        // Calculate BGDT location based on block size
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;  // For 1K blocks, BGDT starts at block 2
        } else {
            BGDT_index = 1;  // For larger blocks, BGDT starts at block 1
        }
        
        // Read the block group descriptor table for group 0
        bgdt = new BGDT();
        adapter->read_all(BGDT_index * (1024 << supa->block_size), sizeof(BGDT), (char*)bgdt);
        
        // Create the root node (inode 2 in ext2)
        root = new Node(1024 << supa->block_size, 2, adapter);
        
        // Print superblock and BGDT info
        printf("Blocks: %u | Inodes: %u | Block size = %u | Inodes per group = %u\n",
            supa->num_Blocks,
            supa->num_iNodes,
            1024 << supa->block_size,
            supa->num_iNode_pergroup);
        
        printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | "
               "num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
            bgdt->bit_map_block_address,
            bgdt->bit_map_iNode_address,
            bgdt->startingBlockAddress,
            bgdt->num_unallocated_blocks,
            bgdt->num_unallocated_directories,
            bgdt->num_unallocated_iNodes);
    }
    
    // Returns the block size of the filesystem
    uint32_t get_block_size() {
        return 1024 << supa->block_size;
    }
    
    // Returns the inode size of the filesystem
    uint32_t get_inode_size() {
        return supa->iNode_size;
    }
};

/*
 * FILESYSTEM OPERATIONS
 *
 * These functions provide the main operations for navigating and reading
 * from the filesystem.
 */

// String comparison utility
bool streq_ext(const char* a, const char* b);

// List the contents of a directory
void list_directory(Node* dir);

// Find an entry in a directory by name
Node* find_in_directory(Node* dir, const char* name);

// Read a file's contents into a buffer
int read_file(Node* file, char* buffer, uint32_t max_size);

// Test the directory traversal functionality
void test_directory_traversal();

// Initialize the filesystem
void init_ext2();

#endif // _EXT2_H_