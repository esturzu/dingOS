#pragma once

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"
#include "sd_adapter.h"

void start_transaction(SDAdapter* adapter);
void log_metadata_block(SDAdapter* adapter, uint32_t fs_block_no, const void* data);
void commit_transaction(SDAdapter* adapter);


struct journal_header;

// Journal Header structure (same format for ext3 and ext4)
struct journal_header {
    uint32_t magic;          // Magic signature: 0xc03b3998 (big endian)
    uint32_t block_type;     // Block type (big endian)
    uint32_t transaction_id; // Transaction ID for this block (big endian)
};

// Journal Superblock structure for ext3 (no checksums, no algorithm field)
struct journal_superblock {
    struct journal_header header; // Bytes 0–11

    uint32_t block_size;            // Block size in bytes [12–15]
    uint32_t total_blocks;          // Total number of blocks [16–19]
    uint32_t first_info_block;      // First block of journal info [20–23]
    uint32_t sequence_number;       // First expected transaction [24–27]
    uint32_t start_block;           // First block of the journal [28–31]
    uint32_t errno;                 // Error indicator [32–35]
    uint32_t feature_compat;        // Required features [36–39]
    uint32_t feature_incompat;      // Optional features [40–43]
    uint32_t feature_ro_compat;     // Read-only features [44–47]

    uint8_t uuid[16];               // UUID of journal [48–63]

    uint32_t user_count;            // Number of filesystems using this journal [64–67]
    uint32_t sb_copy_block;         // Superblock copy block (optional) [68–71]
    uint32_t max_transaction_blocks;// Max journal blocks per transaction (unused) [72–75]
    uint32_t max_data_blocks;       // Max data blocks per transaction (unused) [76–79]

    uint8_t padding[944];           // Padding to fill up 1024 bytes [80–1023]
};

extern void memcpy_ext(void* dest, const void* src, uint32_t n);
extern void init_journal(journal_superblock* loaded_jsb); 
extern void start_transaction(SDAdapter* adapter);
extern void log_metadata_block(SDAdapter* adapter, uint32_t fs_block_no, const void* data);
extern void commit_transaction(SDAdapter* adapter); 
extern uint32_t get_journal_start();


