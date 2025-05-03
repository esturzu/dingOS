#include "journal.h"
#include "physmem.h"
#include "printf.h"
#include "ext2.h"


static uint32_t current_tx_id = 1;
static uint32_t journal_write_ptr = 0;
static uint32_t journal_start = 0;
static uint32_t journal_end = 0;
static journal_superblock* jsb = nullptr;

struct journal_block {
    journal_header header;
    uint32_t target_block;
    uint8_t payload[1024]; // assuming 1KB blocks
};

void memcpy_ext(void* dest, const void* src, uint32_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

//Initializes the journaling system by setting up global variables based on the loaded journal superblock.
void init_journal(journal_superblock* loaded_jsb) {
    jsb = loaded_jsb;
    journal_start = ntohl(jsb->start_block);
    journal_end = journal_start + ntohl(jsb->total_blocks);
    journal_write_ptr = journal_start; // or scan journal to resume
    current_tx_id = ntohl(jsb->sequence_number);
}

//Begins a new transaction by writing a TxStart marker to the journal.
void start_transaction(SDAdapter* adapter) {
    journal_header header{};
    header.magic = htonl(0xc03b3998);
    header.block_type = htonl(1); // TxStart
    header.transaction_id = htonl(current_tx_id);

    adapter->write_block(journal_write_ptr, (char*)&header, 0, sizeof(journal_header));
    printf("Journal: Started transaction %u at block %u\n", current_tx_id, journal_write_ptr);
    journal_write_ptr++;
}

//Logs a 1024-byte metadata block (like an inode or bitmap) to the journal 
//with its corresponding filesystem block number.
void log_metadata_block(SDAdapter* adapter, uint32_t fs_block_no, const void* data) {
    journal_block entry;

    entry.header.magic = htonl(0xc03b3998);
    entry.header.block_type = htonl(2); // Metadata block
    entry.header.transaction_id = htonl(current_tx_id);

    entry.target_block = htonl(fs_block_no);
    memcpy_ext(entry.payload, data, 1024);

    adapter->write_block(journal_write_ptr, (char*)&entry, 0, sizeof(entry));
    printf("Journal: Logged metadata block %u (txn %u) at journal block %u\n", fs_block_no, current_tx_id, journal_write_ptr);
    journal_write_ptr++;
}


//Marks the end of a transaction by writing a TxEnd block to the journal.
void commit_transaction(SDAdapter* adapter) {
    journal_header header;
    header.magic = htonl(0xc03b3998);
    header.block_type = htonl(3); // TxEnd
    header.transaction_id = htonl(current_tx_id);

    adapter->write_block(journal_write_ptr, (char*)&header, 0, sizeof(journal_header));
    printf("Journal: Committed transaction %u at block %u\n", current_tx_id, journal_write_ptr);
    journal_write_ptr++;

    current_tx_id++;
    jsb->sequence_number = htonl(current_tx_id);
}

uint32_t get_journal_start() {
    return journal_start;
}


void replay_journal(SDAdapter* adapter) {
    uint32_t block = journal_start;
    bool in_tx = false;
    uint32_t tx_id = 0;

    while (block < journal_write_ptr) {
        char buf[1024];
        adapter->read_block(block, buf);

        journal_header* header = (journal_header*)buf;

        uint32_t magic = ntohl(header->magic);
        uint32_t type = ntohl(header->block_type);
        uint32_t id = ntohl(header->transaction_id);

        if (magic != 0xc03b3998) {
            printf("Replay: Invalid magic at block %u. Aborting.\n", block);
            break;
        }

        if (type == 1) { // TxStart
            printf("Replay: TxStart txn %u at block %u\n", id, block);
            in_tx = true;
            tx_id = id;
        } else if (type == 2 && in_tx) { // Metadata block
            journal_block* entry = (journal_block*)buf;
            uint32_t fs_block = ntohl(entry->target_block);
            adapter->write_block(fs_block, (char*)entry->payload, 0, 1024);
            printf("Replay: Replayed metadata block to fs block %u (block %u in journal)\n", fs_block, block);
            printf("Replay: First 16 bytes of payload: ");
            for (int i = 0; i < 16; ++i) {
                printf("%02x ", (uint8_t)entry->payload[i]);
            }
            printf("\n");
        } else if (type == 3 && in_tx && id == tx_id) { // TxEnd
            printf("Replay: TxEnd txn %u at block %u\n", id, block);
            in_tx = false;
        }

        block++;
    }

    if (in_tx) {
        printf("Replay: Incomplete txn %u detected at end of journal.\n", tx_id);
    }
    if (!in_tx) {
        printf("Replay: Completed journal replay. Resetting write pointer.\n");
        journal_write_ptr = journal_start;

        // Optionally clear journal blocks
        // for (uint32_t i = journal_start; i < journal_end; i++) {
        //     zero_block(adapter, i);
        // }
    }
}
