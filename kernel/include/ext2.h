#ifndef _EXT2_H_
#define _EXT2_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"

// Constants
#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_SB_OFFSET 1024  // Superblock offset from start of volume

// Filesystem structures
struct super_block {
    uint32_t num_iNodes;
    uint32_t num_Blocks;
    char pad0[16];
    uint32_t block_size;
    char pad1[4];
    uint32_t num_blocks_pergroup;
    char pad2[4];
    uint32_t num_iNode_pergroup;
    char pad3[44];
    uint16_t iNode_size;
};

struct iNode {
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

struct BGDT {
    uint32_t bit_map_block_address;
    uint32_t bit_map_iNode_address;
    uint32_t startingBlockAddress;
    uint16_t num_unallocated_blocks;
    uint16_t num_unallocated_iNodes;
    uint16_t num_unallocated_directories;
    char pad0[14];
};

struct dir_entry {
    uint32_t iNodeNum;
    uint16_t size_entry;
    uint8_t name_length;
    uint8_t type_indicator;
    char name_characters[256]; // Fixed size array instead of char*
};

// Global variables
extern super_block* supa;
extern BGDT* bgdt;
extern uint32_t SB_offset;
extern uint32_t BGDT_index;

// Forward declarations
class Node;

// SD adapter for ext2
class SDAdapter : public BlockIO {
public:
    SDAdapter(uint32_t block_size) : BlockIO(block_size) {}
    
    virtual ~SDAdapter() {}
    
    uint32_t size_in_bytes() override {
        return 0xFFFFFFFF; // Large enough value
    }
    
    void read_block(uint32_t block_number, char* buffer) override {
        uint32_t sd_block_count = block_size / SD::BLOCKSIZE;
        uint32_t sd_start = block_number * sd_block_count;
        uint32_t bytes = SD::read(sd_start, sd_block_count, (uint8_t*)buffer);
        if (bytes != block_size) {
            printf("SDAdapter::read_block: Failed to read block %u\n", block_number);
        }
    }
    
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override {
        // Just a stub for now - not needed for initialization
        printf("SDAdapter::write_block: Not implemented\n");
    }
};

// A wrapper around an i-node
class Node : public BlockIO {
public:
    const uint32_t number;  // i-number of this node
    iNode* node;
    uint32_t block_group;
    uint32_t index;
    uint32_t containing_block;
    uint32_t BGDT_index;
    SDAdapter* sd_adapter;
    bool have_entry_before;
    uint32_t entry_result;
    char* oldSym;

    Node(uint32_t block_size, uint32_t number, SDAdapter* adapter) 
        : BlockIO(block_size), number(number), node(nullptr), 
          sd_adapter(adapter), have_entry_before(false), entry_result(0), oldSym(nullptr) {
        
        // Initialize the node
        node = new iNode();
        
        // Calculate block group and index
        block_group = (number - 1) / supa->num_iNode_pergroup;
        index = (number - 1) % supa->num_iNode_pergroup;
        
        // Set BGDT index based on block size
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;
        } else {
            BGDT_index = 1;
        }
        
        // Read the inode data
        uint32_t inode_offset = bgdt->startingBlockAddress * (1024 << supa->block_size) + index * supa->iNode_size;
        sd_adapter->read_all(inode_offset, sizeof(iNode), (char*)node);
    }
    
    virtual ~Node() {
        delete node;
        if (oldSym) delete[] oldSym;
    }
    
    // How many bytes does this i-node represent
    uint32_t size_in_bytes() override {
        if (is_dir()) {
            // For directories, we'll count entries later
            return 0;
        } else if (is_file()) {
            return node->size_of_iNode;
        } else if (is_symlink()) {
            return node->size_of_iNode;
        }
        return 0;
    }
    
    // Read the given block using direct/indirect blocks
    void read_block(uint32_t block_number, char* buffer) override {
        uint32_t N = (1024 << supa->block_size) / 4;
        
        if (block_number < 12) {
            // Direct block
            sd_adapter->read_all(node->directLinked[block_number] * (1024 << supa->block_size), 
                             (1024 << supa->block_size), buffer);
        } else if (block_number < (12 + N)) {
            // Single indirect block - we'll implement this later
            printf("Node::read_block: Single indirect blocks not implemented\n");
        } else if (block_number < (12 + N + (N * N))) {
            // Double indirect block - we'll implement this later
            printf("Node::read_block: Double indirect blocks not implemented\n");
        } else {
            // Triple indirect block - we'll implement this later
            printf("Node::read_block: Triple indirect blocks not implemented\n");
        }
    }
    
    // Write is not needed for initialization
    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override {
        printf("Node::write_block: Not implemented\n");
    }
    
    // Type checking functions
    uint32_t get_type() {
        // Extract the file type from the modes/permissions field
        uint32_t type = (node->types_plus_perm >> 12) << 12;
        return type;
    }
    
    bool is_dir() {
        uint32_t dir_code = 16384;  // 0x4000
        return (get_type() == dir_code);
    }
    
    bool is_file() {
        uint32_t file_code = 32768;  // 0x8000
        return (get_type() == file_code);
    }
    
    bool is_symlink() {
        uint32_t sym_link = 40960;  // 0xA000
        return (get_type() == sym_link);
    }
};

// Main ext2 filesystem class
class Ext2 {
public:
    Node* root;
    
    Ext2(SDAdapter* adapter) {
        // Read the superblock
        supa = new super_block();
        
        adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
        
        // Verify the filesystem
        // TODO: Add magic number check
        
        // Calculate BGDT location
        if ((1024 << supa->block_size) == 1024) {
            BGDT_index = 2;
        } else {
            BGDT_index = 1;
        }
        
        // Read the block group descriptor table for group 0
        bgdt = new BGDT();
        adapter->read_all(BGDT_index * (1024 << supa->block_size), sizeof(BGDT), (char*)bgdt);
        
        // Create the root node (inode 2 in ext2)
        root = new Node(1024 << supa->block_size, 2, adapter);
        
          // Print superblock info in the exact format requested
        printf("Blocks: %u | Inodes: %u | Block size = %u | Inodes per group = %u\n",
            supa->num_Blocks,
            supa->num_iNodes,
            1024 << supa->block_size,
            supa->num_iNode_pergroup);
        printf("bit_map_block_address: %u | bit_map_iNode_address: %u | startingBlockAddress = %u | num_unallocated_blocks = %u | num_unallocated_directories = %u | num_unallocated_iNodes = %u |\n",
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
    
    // Returns the actual size of an i-node
    uint32_t get_inode_size() {
        return supa->iNode_size;
    }
};


#endif // _EXT2_H_