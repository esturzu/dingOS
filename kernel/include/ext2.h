#ifndef _EXT2_H_
#define _EXT2_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"
#include "sd_adapter.h"

/**
 * EXT2 Filesystem Implementation
 * 
 * This implementation provides access to EXT2 filesystems on an SD card.
 * The EXT2 filesystem organizes disk space into block groups to minimize seek time
 * and improve performance by keeping related data close together physically.
 * 
 * Key components:
 * - Superblock: Contains filesystem metadata (block size, inode count, etc.)
 * - Block Group Descriptor Table (BGDT): Describes each block group
 * - Inodes: Metadata structures that represent files, directories, or symlinks
 * - Data Blocks: Store actual file data or directory entries
 * - Bitmaps: Track which blocks and inodes are allocated/free
 * 
 * Design overview:
 * - SDAdapter: Bridges between BlockIO and SD card operations
 * - Node: Represents files, directories, and symlinks
 * - Ext2: Main filesystem class that mounts and manages the filesystem
 */

// Constants for the ext2 filesystem
#define EXT2_SUPER_MAGIC 0xEF53    // Magic signature for ext2 filesystems (used for validation)
#define EXT2_SB_OFFSET 1024        // Superblock is located 1KB from start of volume

/**
 * FILESYSTEM STRUCTURES
 * 
 * These structures match the on-disk layout of ext2 filesystem structures.
 * All fields are kept in their original order and size to match the ext2 specification.
 * Note: All values are stored in little-endian byte order on disk.
 */

/**
 * Superblock structure - contains filesystem metadata
 * 
 * The superblock is the central data structure that describes the filesystem layout
 * and properties. It's located at byte offset 1024 from the start of the volume.
 * Backup copies are stored in various block groups for redundancy.
 */
struct super_block {
    uint32_t num_iNodes;           // Total number of inodes in filesystem
    uint32_t num_Blocks;           // Total number of blocks in filesystem
    char pad0[16];                 // Reserved/unused fields
    uint32_t block_size;           // Block size as a power of 2 (0=1K, 1=2K, 2=4K, etc.)
    char pad1[4];                  // Reserved/unused fields
    uint32_t num_blocks_pergroup;  // Number of blocks per block group
    char pad2[4];                  // Reserved/unused fields
    uint32_t num_iNode_pergroup;   // Number of inodes per block group
    char pad3[44];                 // Reserved/unused fields
    uint16_t iNode_size;           // Size of each inode structure in bytes
    uint16_t magic; 
};

/**
 * iNode structure - represents a file, directory, or symlink
 * 
 * An inode contains metadata about a file, directory, or symlink, including:
 * - Type and permissions
 * - Size
 * - Block pointers (direct, indirect, double and triple indirect)
 * - Ownership, timestamps, etc. (many fields omitted in this implementation)
 * 
 * The inode does not store the filename - filenames are stored in directory entries
 * that point to inodes.
 */
struct iNode {
    uint16_t types_plus_perm;      // Type (file/dir/symlink) and permissions (highest 4 bits = type)
    char pad0[2];                  // Reserved/unused fields
    uint32_t size_of_iNode;        // Size in bytes of the file/directory
    char pad1[18];                 // Reserved/unused fields (includes timestamps, uid/gid, etc.)
    uint16_t num_Hard_Links;       // Number of hard links to this inode
    char pad2[12];                 // Reserved/unused fields
    uint32_t directLinked[12];     // Direct block pointers (first 12 blocks of data)
    uint32_t singleIndirect;       // Single indirect block pointer (points to a block of pointers)
    uint32_t doubleIndirect;       // Double indirect block pointer (points to blocks of pointers to blocks)
    uint32_t tripleIndirect;       // Triple indirect block pointer (additional level of indirection)
    char pad3[28];                 // Reserved/unused fields
};

/**
 * Block Group Descriptor Table entry - describes a block group
 * 
 * The filesystem is divided into block groups to improve performance.
 * Each block group has its own descriptor entry in the BGDT that points
 * to key data structures within that group.
 */
struct BGDT {
    uint32_t bit_map_block_address;    // Block number containing the block bitmap
    uint32_t bit_map_iNode_address;    // Block number containing the inode bitmap
    uint32_t startingBlockAddress;     // Block number of the first inode table block
    uint16_t num_unallocated_blocks;   // Number of unallocated blocks in this group
    uint16_t num_unallocated_iNodes;   // Number of unallocated inodes in this group
    uint16_t num_unallocated_directories; // Number of directories in this group
    char pad0[14];                     // Reserved/unused fields
};

/**
 * Directory entry structure - links names to inodes
 * 
 * A directory is a special file containing directory entries.
 * Each entry associates a filename with an inode number and includes
 * the size of the entry and the type of the file.
 */
struct dir_entry {
    uint32_t iNodeNum;             // Inode number this entry points to
    uint16_t size_entry;           // Total size of this entry (including name) in bytes
    uint8_t name_length;           // Length of the name in bytes
    uint8_t type_indicator;        // Type of file (optional in some ext2 versions)
    char name_characters[256];     // Name (variable length, null-terminated for convenience)
                                   // Note: Actual on-disk size is determined by name_length
};

// Global variables accessible throughout the filesystem
extern super_block* supa;          // Pointer to the mounted filesystem's superblock
extern BGDT* bgdt;                 // Pointer to the first block group descriptor
extern uint32_t SB_offset;         // Offset to the superblock (1024 bytes)
extern uint32_t BGDT_index;        // Block index of the BGDT
extern uint8_t* cached_inode_bitmap; // Cached bitmap of allocated inodes
extern uint8_t* cached_block_bitmap; // Cached bitmap of allocated blocks

// Forward declarations
class Node;

/**
 * NODE CLASS
 * 
 * This class represents an inode in the filesystem (file, directory, or symlink).
 * It inherits from BlockIO to provide block-level access to the node's data.
 * Each Node instance corresponds to one inode in the filesystem.
 */
class Node : public BlockIO {
public:
    const uint32_t number;         // Inode number (1-based)
    iNode* node;                   // Pointer to the in-memory inode data
    uint32_t block_group;          // Block group containing this inode
    uint32_t index;                // Index of this inode within its block group
    uint32_t containing_block;     // Block containing this inode
    uint32_t BGDT_index;           // Block index of the BGDT
    SDAdapter* sd_adapter;         // Pointer to the SD adapter
    bool have_entry_before;        // Flag for directory entry count
    uint32_t entry_result;         // Directory entry count
    char* oldSym;                  // Symlink content buffer

    /**
     * Constructor loads an inode from disk
     * 
     * @param block_size Size of filesystem blocks in bytes
     * @param number Inode number to load
     * @param adapter Pointer to the SD adapter for disk access
     */
    Node(uint32_t block_size, uint32_t number, SDAdapter* adapter) 
        : BlockIO(block_size), number(number), node(nullptr), 
          sd_adapter(adapter), have_entry_before(false), entry_result(0), oldSym(nullptr) {
        
        debug_printf("DEBUG: Node constructor: Creating Node for inode %u\n", number);
        
        // Allocate and initialize the inode
        node = new iNode();
        
        // Calculate which block group contains this inode
        // Inodes are numbered starting from 1, so subtract 1 first
        // Then divide by the number of inodes per group to get group index
        block_group = (number - 1) / supa->num_iNode_pergroup;
        
        // Calculate index within the block group
        // This is the remainder after division, giving position within the group
        index = (number - 1) % supa->num_iNode_pergroup;
        
        // Set BGDT index based on block size
        // For 1K blocks, BGDT starts at block 2
        // For larger blocks, BGDT starts at block 1
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;
        } else {
            BGDT_index = 1;
        }
        
        // Read the inode data from disk
        // Calculate the byte offset to the inode:
        // 1. Start with the block containing the inode table (bgdt->startingBlockAddress)
        // 2. Multiply by block size to get byte offset to start of table
        // 3. Add (index * iNode_size) to get offset to specific inode
        uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + 
                               index * supa->iNode_size;
        
        debug_printf("DEBUG: Node constructor: Reading inode data from offset 0x%x\n", inode_offset);
        sd_adapter->read_all(inode_offset, sizeof(iNode), (char*)node);
        debug_printf("DEBUG: Node constructor: Inode loaded, type=0x%x, size=%u\n", 
              node->types_plus_perm, node->size_of_iNode);
    }
    
    /**
     * Destructor cleans up allocated memory
     */
    virtual ~Node() {
        delete node;
        if (oldSym) delete[] oldSym;
    }
    
    /**
     * Returns the size of this node in bytes
     * 
     * The size depends on the node type:
     * - For files and directories, it's stored in the inode
     * - For symlinks, it's the length of the link target
     * 
     * @return Size in bytes
     */
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
    
    /**
     * Read a block from this node
     * 
     * This method handles the different levels of indirection:
     * - Direct blocks (0-11): accessed directly from the inode
     * - Single indirect blocks (12+): require one level of indirection
     * - Double indirect blocks: require two levels of indirection
     * - Triple indirect blocks: require three levels of indirection
     * 
     * @param block_number Logical block number within the file
     * @param buffer Buffer to store the read data
     */
    void read_block(uint32_t block_number, char* buffer) override {
        debug_printf("DEBUG: Node::read_block: inode %u, block %u\n", number, block_number);
        
        // Calculate how many block pointers fit in one block
        // Each pointer is 4 bytes (uint32_t), so divide block size by 4
        uint32_t N = (1024 << supa->block_size) / 4;
        
        if (block_number < 12) {
            // Direct block - directly referenced by the inode
            // The first 12 blocks are pointed to directly by the inode
            if (node->directLinked[block_number] == 0) {
                debug_printf("DEBUG: Node::read_block: Block is not allocated (zero)\n");
                // zero_memory(buffer, block_size);
                return;
            }
            
            debug_printf("DEBUG: Node::read_block: Reading direct block at physical block %u\n", 
                  node->directLinked[block_number]);
            
            // Calculate byte offset: block_number * block_size
            // Read the entire block into the buffer
            sd_adapter->read_all(node->directLinked[block_number] * (1024 << supa->block_size), 
                            (1024 << supa->block_size), buffer);
        } else if (block_number < (12 + N)) {
            // Single indirect block - referenced through a single indirection
            // The block_number is beyond the direct blocks, so we need to use the single indirect pointer
            // This implementation currently doesn't support single indirect blocks
            debug_printf("DEBUG: Node::read_block: Single indirect blocks not implemented\n");
        } else if (block_number < (12 + N + (N * N))) {
            // Double indirect block - referenced through double indirection
            // This requires reading two levels of block pointers to find the data
            debug_printf("DEBUG: Node::read_block: Double indirect blocks not implemented\n");
        } else {
            // Triple indirect block - referenced through triple indirection
            // This requires reading three levels of block pointers to find the data
            debug_printf("DEBUG: Node::read_block: Triple indirect blocks not implemented\n");
        }
    }
    
    /**
     * Write a block to this node
     * 
     * Similar to read_block, but handles writing data instead of reading.
     * If the block doesn't exist yet, it must be allocated first.
     * 
     * @param block_number Logical block number within the file
     * @param buffer Data to write
     * @param offset Offset within the block to start writing
     * @param n Number of bytes to write
     */
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override;

    /**
     * Update inode on disk after changes
     * 
     * When the in-memory inode is modified, this method writes the
     * changes back to disk to persist them.
     */
    void update_inode_on_disk();

    /**
     * Find and allocate a free block
     * 
     * Searches the block bitmap for an unallocated block, marks it
     * as allocated, and returns its block number.
     * 
     * @return Allocated block number, or 0 if no free blocks
     */
    uint32_t allocate_block();
    
    /**
     * Extract the file type from the mode/permissions field
     * 
     * The high 4 bits of types_plus_perm field contain the file type:
     * - 0x8000: Regular file
     * - 0x4000: Directory
     * - 0xA000: Symbolic link
     * - Others: Various special file types (devices, sockets, etc.)
     * 
     * @return The file type bits (0xX000, where X is the 4-bit type code)
     */
    uint32_t get_type() {
        // Print debug information
        debug_printf("DEBUG: Types and permissions raw value: 0x%x\n", node->types_plus_perm);
        
        // Extract type bits (top 4 bits) using bitwise AND with 0xF000 mask
        // This masks off the permission bits (lower 12 bits) and keeps only the type
        uint32_t type = node->types_plus_perm & 0xF000;
        return type;
    }
    
    /**
     * Check if this node is a directory
     * 
     * @return true if this is a directory, false otherwise
     */
    bool is_dir() {
        uint32_t dir_code = 16384;  // 0x4000 in decimal
        return (get_type() == dir_code);
    }
    
    /**
     * Check if this node is a regular file
     * 
     * @return true if this is a regular file, false otherwise
     */
    bool is_file() {
        uint32_t file_code = 0x8000;  // Regular file
        uint32_t type = get_type();
        debug_printf("DEBUG: is_file() type check: raw=0x%x, expected=0x%x\n", type, file_code);
        return (type == file_code);
    }
    
    /**
     * Check if this node is a symbolic link
     * 
     * @return true if this is a symbolic link, false otherwise
     */
    bool is_symlink() {
        uint32_t sym_link = 40960;  // 0xA000 in decimal
        return (get_type() == sym_link);
    }
    
    /**
     * Write data to the node starting at a specified offset
     * 
     * This method calculates which block the write operation affects,
     * and delegates the actual writing to write_block.
     * 
     * @param offset Byte offset within the file to start writing
     * @param n Number of bytes to write
     * @param buffer Data to write
     * @return Number of bytes written, or negative value on error
     */
    int64_t write(uint32_t offset, uint32_t n, char* buffer) override {
        debug_printf("DEBUG: Node::write: inode %u, offset %u, size %u\n", number, offset, n);
        
        // Calculate which block and offset within block
        // Integer division gives block number: offset / block_size
        uint32_t block_number = offset / block_size;
        
        // Modulo operation gives position within block: offset % block_size
        uint32_t offset_in_block = offset % block_size;
        
        // Calculate how many bytes we can write in this block
        // Either the requested size or the remaining space in the block
        uint32_t bytes_to_write = (n < block_size - offset_in_block) ? n : block_size - offset_in_block;
        
        debug_printf("DEBUG: Node::write: Writing to block %u, offset_in_block %u, bytes_to_write %u\n", 
              block_number, offset_in_block, bytes_to_write);
        
        // Write to the block
        write_block(block_number, buffer, offset_in_block, bytes_to_write);
        
        return bytes_to_write;
    }
    
    /**
     * Write all data to the node
     * 
     * Makes multiple calls to write() to ensure all requested data is written,
     * even if it spans multiple blocks.
     * 
     * @param offset Byte offset within the file to start writing
     * @param n Number of bytes to write
     * @param buffer Data to write
     * @return Total number of bytes written, or negative value on error
     */
    int64_t write_all(uint32_t offset, uint32_t n, char* buffer) override {
        debug_printf("DEBUG: Node::write_all: inode %u, offset %u, size %u\n", number, offset, n);
        debug_printf("DEBUG: Node::write_all: Before write, inode size=%u\n", node->size_of_iNode);
        
        int64_t total_count = 0;
        while (n > 0) {
            // Write as much as we can at the current offset
            int64_t cnt = write(offset, n, buffer);
            debug_printf("DEBUG: Node::write_all: write returned %lld\n", cnt);
            
            // Check for errors or end of file
            if (cnt < 0) return cnt;
            if (cnt == 0) break;
            
            // Update counters for next iteration
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

/**
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
    
    /**
     * Constructor mounts the filesystem
     * 
     * Reads the superblock and block group descriptor table,
     * and creates a Node instance for the root directory.
     * 
     * @param adapter Pointer to the SD adapter for disk access
     */
    Ext2(SDAdapter* adapter) : adapter(adapter) {
        // Read the superblock from its fixed location (1KB offset)
        supa = new super_block();
        adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
        
        // Verify the filesystem magic number (TODO)
        // A proper implementation would check for the magic number (0xEF53)
        
        // Calculate BGDT location based on block size
        // For 1K blocks, BGDT starts at block 2 (after superblock)
        // For larger blocks, BGDT starts at block 1 (superblock is in block 0)
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;  // For 1K blocks, BGDT starts at block 2
        } else {
            BGDT_index = 1;  // For larger blocks, BGDT starts at block 1
        }
        
        // Read the block group descriptor table for group 0
        bgdt = new BGDT();
        adapter->read_all(BGDT_index * (1024 << supa->block_size), sizeof(BGDT), (char*)bgdt);
        
        // Create the root node (inode 2 in ext2)
        // Inode 2 is always the root directory in ext2
        root = new Node(1024 << supa->block_size, 2, adapter);
        
        // Print superblock and BGDT info for debugging
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
        
        // Allocate memory for bitmap caches
        cached_inode_bitmap = new uint8_t[1024 << supa->block_size];
        cached_block_bitmap = new uint8_t[1024 << supa->block_size];
    }
    
    /**
     * Destructor cleans up allocated memory
     */
    ~Ext2() {
        delete root;
        delete supa;
        delete bgdt;
        delete cached_inode_bitmap;
        delete cached_block_bitmap;
    }
    
    /**
     * Returns the block size of the filesystem in bytes
     * 
     * The block_size field in the superblock is stored as a power of 2,
     * with 0 meaning 1K, 1 meaning 2K, 2 meaning 4K, etc.
     * 
     * @return Block size in bytes
     */
    uint32_t get_block_size() {
        // 1024 << supa->block_size calculates 1024 * 2^(block_size)
        return 1024 << supa->block_size;
    }
    
    /**
     * Returns the inode size of the filesystem in bytes
     * 
     * @return Size of each inode structure in bytes
     */
    uint32_t get_inode_size() {
        return supa->iNode_size;
    }
};

/**
 * FILESYSTEM OPERATIONS
 *
 * These functions provide the main operations for navigating and reading
 * from the filesystem.
 */

/**
 * Get string length (safe replacement for strlen)
 * 
 * @param str Null-terminated string
 * @return Length of the string in bytes
 */
uint32_t strlen_ext(const char* str);

/**
 * Fill memory with zeros (safe replacement for memset)
 * 
 * @param buffer Memory to clear
 * @param size Number of bytes to zero
 */
void zero_memory(void* buffer, uint32_t size);

/**
 * Dump a block's contents for debugging
 * 
 * @param adapter Pointer to the SD adapter
 * @param block_start First block to dump
 * @param count Number of blocks to dump
 */
void dump_blocks(SDAdapter* adapter, uint32_t block_start, uint32_t count);

/**
 * String comparison utility
 * 
 * @param a First string to compare
 * @param b Second string to compare
 * @return true if strings are equal, false otherwise
 */
bool streq_ext(const char* a, const char* b);

/**
 * List the contents of a directory
 * 
 * @param dir Node representing the directory to list
 */
void list_directory(Node* dir);

/**
 * Find an entry in a directory by name
 * 
 * @param dir Node representing the directory to search
 * @param name Name of the entry to find
 * @return Node representing the found entry, or nullptr if not found
 */
Node* find_in_directory(Node* dir, const char* name);

/**
 * Read a file's contents into a buffer
 * 
 * @param file Node representing the file to read
 * @param buffer Buffer to store the file contents
 * @param max_size Maximum number of bytes to read
 * @return Number of bytes read, or negative value on error
 */
int read_file(Node* file, char* buffer, uint32_t max_size);

/**
 * Create a new file in a directory
 * 
 * @param dir Node representing the parent directory
 * @param name Name of the new file
 * @return Node representing the new file, or nullptr on error
 */
Node* create_file(Node* dir, const char* name);

/**
 * Allocate a new inode from the inode bitmap
 * 
 * @param dir Node representing the directory (used to determine block group)
 * @return Allocated inode number, or 0 if no free inodes
 */
uint32_t allocate_inode(Node* dir);

/**
 * Initialize a new inode with the given type
 * 
 * @param inode_num Inode number to initialize
 * @param type Type of inode (0x8000 for file, 0x4000 for directory, etc.)
 * @param adapter Pointer to the SD adapter for disk access
 */
void create_inode(uint32_t inode_num, uint16_t type, SDAdapter* adapter);

/**
 * Add a directory entry for a new file/directory
 * 
 * @param dir Node representing the parent directory
 * @param name Name of the new entry
 * @param inode_num Inode number of the new entry
 */
void add_dir_entry(Node* dir, const char* name, uint32_t inode_num);

#endif // _EXT2_H_