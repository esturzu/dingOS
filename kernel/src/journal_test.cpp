#include "ext2.h"
#include "journal.h"
#include "sd_adapter.h"
#include "printf.h"

void run_journal_test() {
    printf("\n=== Starting Journal Journaling Test ===\n");

    // Step 1: Init SD
    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD card initialization failed!\n");
        return;
    }

    SDAdapter* adapter = new SDAdapter(1024);

    // Step 2: Read superblock
    supa = new super_block();
    adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);

    // Step 3: Update block size and SD adapter
    uint32_t fs_block_size = 1024 << supa->block_size;
    adapter = new SDAdapter(fs_block_size);

    // Step 4: Read BGDT
    bgdt = new BGDT();
    BGDT_index = (fs_block_size == 1024) ? 2 : 1;
    adapter->read_all(BGDT_index * fs_block_size, sizeof(BGDT), (char*)bgdt);

    // Step 5: Init bitmaps
    cached_inode_bitmap = new uint8_t[fs_block_size];
    cached_block_bitmap = new uint8_t[fs_block_size];
    adapter->read_block(bgdt->bit_map_iNode_address, (char*)cached_inode_bitmap);
    adapter->read_block(bgdt->bit_map_block_address, (char*)cached_block_bitmap);

    // Step 6: Init root directory
    Node* root = new Node(fs_block_size, 2, adapter);

    // Step 7: Init journal
    Node* journal_inode = new Node(fs_block_size, 8, adapter);
    uint32_t journal_block = journal_inode->node->directLinked[0];
    journal_superblock* jsb = (journal_superblock*) operator new(sizeof(journal_superblock));
    zero_memory(jsb, sizeof(journal_superblock));
    adapter->read_all(journal_block * fs_block_size, sizeof(journal_superblock), (char*)jsb);

    if (ntohl(jsb->header.magic) != 0xc03b3998) {
        printf("ERROR: Invalid or missing journal superblock\n");
        return;
    }

    init_journal(jsb);
    

    // Step 8: Create and write file
    const char* filename = "journal_test.txt";
    Node* file = create_file(root, filename);
    if (!file) {
        printf("ERROR: Could not create file\n");
        return;
    }

    const char* content = "hello journaled world";
    file->write_all(0, strlen_ext(content), (char*)content);
    delete file;

    // Step 9: Dump journal state
    printf("\n=== Dumping Journal Contents ===\n");
    dump_blocks(adapter, get_journal_start(), 10);

    // Step 10: Simulate crash
    printf("\n!!! Simulate crash now: kill QEMU (Ctrl+A X or close window) !!!\n");
    while (true) {}
}


void verify_journal_recovery() {
    printf("\n=== Verifying Journal Recovery ===\n");

    if (SD::init() != SD::SUCCESS) {
        printf("ERROR: SD init failed during recovery!\n");
        return;
    }

    SDAdapter* adapter = new SDAdapter(1024);
    supa = new super_block();
    adapter->read_all(EXT2_SB_OFFSET, sizeof(super_block), (char*)supa);
    uint32_t fs_block_size = 1024 << supa->block_size;
    adapter = new SDAdapter(fs_block_size);

    bgdt = new BGDT();
    BGDT_index = (fs_block_size == 1024) ? 2 : 1;
    adapter->read_all(BGDT_index * fs_block_size, sizeof(BGDT), (char*)bgdt);

    cached_inode_bitmap = new uint8_t[fs_block_size];
    cached_block_bitmap = new uint8_t[fs_block_size];
    adapter->read_block(bgdt->bit_map_iNode_address, (char*)cached_inode_bitmap);
    adapter->read_block(bgdt->bit_map_block_address, (char*)cached_block_bitmap);

    Node* root = new Node(fs_block_size, 2, adapter);

    // Step 1: Init journal
    Node* journal_inode = new Node(fs_block_size, 8, adapter);
    uint32_t journal_block = journal_inode->node->directLinked[0];
    journal_superblock* jsb = (journal_superblock*) operator new(sizeof(journal_superblock));
    zero_memory(jsb, sizeof(journal_superblock));
    adapter->read_all(journal_block * fs_block_size, sizeof(journal_superblock), (char*)jsb);

    if (ntohl(jsb->header.magic) != 0xc03b3998) {
        printf("ERROR: Invalid or missing journal superblock after recovery\n");
        return;
    }

    init_journal(jsb);
    scan_and_recover();

    // Step 2: Look for journaled file
    Node* file = find_in_directory(root, "journal_test.txt");

    if (!file) {
        printf("FAIL: journal_test.txt not found after recovery\n");
    } else {
        printf("SUCCESS: journal_test.txt found. Reading contents...\n");
        char buffer[64];
        int len = read_file(file, buffer, sizeof(buffer));
        if (len > 0) {
            buffer[len] = '\0';
            printf("Recovered content: %s\n", buffer);
        } else {
            printf("FAIL: Could not read contents of journal_test.txt\n");
        }
        delete file;
    }

    printf("\n=== Post-Recovery Journal Dump ===\n");
    dump_blocks(adapter, get_journal_start(), 10);
}

