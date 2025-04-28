#ifndef _EXT2_H_
#define _EXT2_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"
#include "sd_adapter.h"

/*
 * EXT2 Filesystem Implementation
 * 
 * This implementation provides access to EXT2 filesystems on an SD card.
 * It includes functionality to:
 * - Read the superblock and block group descriptor table
 * - Navigate the directory structure
 * - Read and write files
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
    bool have_entry_before;        // flag for directory entry count
    uint32_t entry_result;         // directory entry count
    char* oldSym;                  // symlink content

    // Constructor loads an inode from disk
    Node(uint32_t block_size, uint32_t number, SDAdapter* adapter) 
        : BlockIO(block_size), number(number), node(nullptr), 
          sd_adapter(adapter), have_entry_before(false), entry_result(0), oldSym(nullptr) {
        
        debug_printf("DEBUG: Node constructor: Creating Node for inode %u\n", number);
        
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
        
        debug_printf("DEBUG: Node constructor: Reading inode data from offset 0x%x\n", inode_offset);
        sd_adapter->read_all(inode_offset, sizeof(iNode), (char*)node);
        debug_printf("DEBUG: Node constructor: Inode loaded, type=0x%x, size=%u\n", 
              node->types_plus_perm, node->size_of_iNode);
    }
    
    // Destructor cleans up allocated memory
    virtual ~Node() {
        delete node;
        if (oldSym) delete[] oldSym;
    }
    
    // Returns the size of this node in bytes
    uint32_t size_in_bytes() override {
        if (is_dir()) {
            // For directories, use the stored size
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
        debug_printf("DEBUG: Node::read_block: inode %u, block %u\n", number, block_number);
        
        // Calculate how many pointers fit in one block
        uint32_t N = (1024 << supa->block_size) / 4;
        
        if (block_number < 12) {
            // Direct block - directly referenced by the inode
            if (node->directLinked[block_number] == 0) {
                debug_printf("DEBUG: Node::read_block: Block is not allocated (zero)\n");
                // zero_memory(buffer, block_size);
                return;
            }
            
            debug_printf("DEBUG: Node::read_block: Reading direct block at physical block %u\n", 
                  node->directLinked[block_number]);
            sd_adapter->read_all(node->directLinked[block_number] * (1024 << supa->block_size), 
                            (1024 << supa->block_size), buffer);
        } else if (block_number < (12 + N)) {
            // Single indirect block - referenced through a single indirection
            debug_printf("DEBUG: Node::read_block: Single indirect blocks not implemented\n");
        } else if (block_number < (12 + N + (N * N))) {
            // Double indirect block - referenced through double indirection
            debug_printf("DEBUG: Node::read_block: Double indirect blocks not implemented\n");
        } else {
            // Triple indirect block - referenced through triple indirection
            debug_printf("DEBUG: Node::read_block: Triple indirect blocks not implemented\n");
        }
    }
    
    // Write a block to this node (implementation in .cpp file)
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override;

    // Update inode on disk after changes
    void update_inode_on_disk();

    // Find and allocate a free block
    uint32_t allocate_block();
    
    // Extract the file type from the mode/permissions field
    uint32_t get_type() {
        // Print debug information
        debug_printf("DEBUG: Types and permissions raw value: 0x%x\n", node->types_plus_perm);
        
        // Extract type bits (top 4 bits) using bitwise AND
        uint32_t type = node->types_plus_perm & 0xF000;
        return type;
    }
    
    // Check if this node is a directory
    bool is_dir() {
        uint32_t dir_code = 16384;  // 0x4000
        return (get_type() == dir_code);
    }
    
    // Check if this node is a regular file
    bool is_file() {
        uint32_t file_code = 0x8000;  // Regular file
        uint32_t type = get_type();
        debug_printf("DEBUG: is_file() type check: raw=0x%x, expected=0x%x\n", type, file_code);
        return (type == file_code);
    }
    
    // Check if this node is a symbolic link
    bool is_symlink() {
        uint32_t sym_link = 40960;  // 0xA000
        return (get_type() == sym_link);
    }
    
    // Override the write method to add debugging
    int64_t write(uint32_t offset, uint32_t n, char* buffer) override {
        debug_printf("DEBUG: Node::write: inode %u, offset %u, size %u\n", number, offset, n);
        
        // Calculate which block and offset within block
        uint32_t block_number = offset / block_size;
        uint32_t offset_in_block = offset % block_size;
        
        // Calculate how many bytes we can write in this block
        uint32_t bytes_to_write = (n < block_size - offset_in_block) ? n : block_size - offset_in_block;
        
        debug_printf("DEBUG: Node::write: Writing to block %u, offset_in_block %u, bytes_to_write %u\n", 
              block_number, offset_in_block, bytes_to_write);
        
        // Write to the block
        write_block(block_number, buffer, offset_in_block, bytes_to_write);
        
        return bytes_to_write;
    }
    
    // Override the write_all method to add debugging
    int64_t write_all(uint32_t offset, uint32_t n, char* buffer) override {
        debug_printf("DEBUG: Node::write_all: inode %u, offset %u, size %u\n", number, offset, n);
        debug_printf("DEBUG: Node::write_all: Before write, inode size=%u\n", node->size_of_iNode);
        
        int64_t total_count = 0;
        while (n > 0) {
            int64_t cnt = write(offset, n, buffer);
            debug_printf("DEBUG: Node::write_all: write returned %lld\n", cnt);
            
            if (cnt < 0) return cnt;
            if (cnt == 0) break;
            
            total_count += cnt;
            offset += cnt;
            n -= cnt;
            buffer += cnt;
        }
        
        debug_printf("DEBUG: Node::write_all: After write, total bytes written=%lld, inode size=%u\n", 
              total_count, node->size_of_iNode);
        return total_count;
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
    SDAdapter* adapter; // Store the adapter for later use
    
    // Constructor mounts the filesystem
    Ext2(SDAdapter* adapter) : adapter(adapter) {
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
        debug_printf("Blocks: %u | Inodes: %u | Block size = %u | Inodes per group = %u\n",
            supa->num_Blocks,
            supa->num_iNodes,
            1024 << supa->block_size,
            supa->num_iNode_pergroup);
        
        debug_printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | "
               "num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
            bgdt->bit_map_block_address,
            bgdt->bit_map_iNode_address,
            bgdt->startingBlockAddress,
            bgdt->num_unallocated_blocks,
            bgdt->num_unallocated_directories,
            bgdt->num_unallocated_iNodes);
    }
    
    // Destructor
    ~Ext2() {
        delete root;
        delete supa;
        delete bgdt;
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
// Get string length (safe replacement for strlen)
uint32_t strlen_ext(const char* str);

// Fill memory with zeros (safe replacement for memset)
void zero_memory(void* buffer, uint32_t size);

// Dump a block's contents for debugging
void dump_blocks(SDAdapter* adapter, uint32_t block_start, uint32_t count);
// String comparison utility
bool streq_ext(const char* a, const char* b);

// List the contents of a directory
void list_directory(Node* dir);

// Find an entry in a directory by name
Node* find_in_directory(Node* dir, const char* name);

// Find an entry (recursively) from a path and a starting node
Node* find_from_path(Node* dir, const char* path);

// Find an entry (recursively) from an absolute path (starting with '/')
Node* find_from_abs_path(const char* path);

// Read a file's contents into a buffer
int read_file(Node* file, char* buffer, uint32_t max_size);

// Create a new file in a directory
Node* create_file(Node* dir, const char* name);

// Allocate a new inode from the inode bitmap
uint32_t allocate_inode(Node* dir);

// Initialize a new inode with the given type
void create_inode(uint32_t inode_num, uint16_t type, SDAdapter* adapter);

// Add a directory entry for a new file/directory
void add_dir_entry(Node* dir, const char* name, uint32_t inode_num);

// Test the directory traversal functionality
void test_directory_traversal();

// Test creating and writing to a file
void test_file_creation();

// Initialize the filesystem
void init_ext2();

void diagnose_block_bitmap(SDAdapter* adapter);

#endif // _EXT2_H_
