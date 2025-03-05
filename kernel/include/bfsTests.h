#ifndef BFS_TESTS_H
#define BFS_TESTS_H

#include "bfs.h"
#include "printf.h"
#include "testFramework.h"

// Helper function to compare buffers manually
// This function checks if two buffers are identical byte by byte.
// It is used to verify that read operations correctly retrieve written data.
bool compareBuffers(const uint8_t* a, const uint8_t* b, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

// BFS Tests
void bfsTests() {
  initTests("BFS Tests");

  // Initialize the Baby File System (BFS)
  // This resets all filesystem structures, including the superblock, file table, and inodes.
  fs_init();

  // Allocate buffers for writing and reading data from the filesystem.
  // These will be used across multiple tests.
  uint8_t* write_buffer = new uint8_t[SD::BLOCKSIZE];
  uint8_t* read_buffer = new uint8_t[SD::BLOCKSIZE]();

  // Fill the write buffer with a repeating pattern of A-Z.
  // This ensures that written data is easily recognizable and verifiable when read back.
  for (size_t i = 0; i < SD::BLOCKSIZE - 1; i++) {
    write_buffer[i] = 'A' + (i % 26);
  }
  write_buffer[SD::BLOCKSIZE - 1] = '\0';  // Null-terminate as a safeguard.

  // === TEST 1: Create and Fill File System ===
  // This test attempts to create the maximum number of files allowed by BFS (`MAX_FILES`).
  // It ensures that:
  // 1. File creation works as expected.
  // 2. The system correctly enforces the file limit.
  // Limitations:
  // - This test does not check if files are allocated at expected locations.
  // - Does not verify if previously created files persist after a reboot.
  char filename[MAX_FILENAME];
  int files_created = 0;
  bool create_success = true;

  for (int i = 0; i < MAX_FILES; i++) {
    strncpy(filename, "file", MAX_FILENAME);
    filename[4] = '0' + (i / 10);  // Convert number to character for filenames.
    filename[5] = '0' + (i % 10);
    filename[6] = '\0';

    if (fs_create(filename, SD::BLOCKSIZE) == 0) {
      files_created++;  // Keep track of how many files were successfully created.
    } else {
      create_success = false;  // File creation failed (probably hit the limit).
      break;
    }
  }

  testsResult("Create and Fill File System", create_success);

  // === TEST 2: Write to All Files ===
  // This test writes `SD::BLOCKSIZE` bytes to each successfully created file.
  // It ensures that:
  // 1. The `fs_write()` function works correctly for single-block writes.
  // 2. Files are not corrupted when written in sequence.
  // Limitations:
  // - This test only writes one block per file, so it does not test multi-block writes.
  // - No verification is performed at this stage (verification happens in Test 3).
  bool write_success = true;
  for (int i = 0; i < files_created; i++) {
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);

    if (fs_write(filename, reinterpret_cast<char*>(write_buffer), SD::BLOCKSIZE) != 0) {
      write_success = false;
      break;
    }
  }

  testsResult("Write to All Files", write_success);

  // === TEST 3: Read Back and Verify Data ===
  // This test reads back all written files and compares them to the expected data.
  // It ensures that:
  // 1. `fs_read()` correctly retrieves data from files.
  // 2. Data integrity is preserved (i.e., the written pattern matches the read pattern).
  // Limitations:
  // - This test only verifies single-block reads.
  // - Does not check partial reads or reads of smaller sizes.
  bool read_success = true;
  for (int i = 0; i < files_created; i++) {
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);

    // Clear read buffer manually to avoid false positives.
    for (size_t j = 0; j < SD::BLOCKSIZE; j++) {
      read_buffer[j] = 0;
    }

    int read_response = fs_read(filename, reinterpret_cast<char*>(read_buffer));
    if (read_response <= 0 || !compareBuffers(write_buffer, read_buffer, SD::BLOCKSIZE)) {
      read_success = false;
      break;
    }
  }

  testsResult("Read and Verify Data Integrity", read_success);

  // === TEST 4: Overwrite First File ===
  // This test overwrites `file00` with a new message and verifies the change.
  // It ensures that:
  // 1. Files can be successfully overwritten.
  // 2. The new data replaces the old content correctly.
  // Limitations:
  // - This test only overwrites a single file (`file00`).
  // - Does not check for fragmentation or handling of partially overwritten blocks.
  const char* overwrite_msg = "Overwritten!";
  for (size_t i = 0; i < SD::BLOCKSIZE; i++) {
    write_buffer[i] = 0;  // Clear buffer before writing new data.
  }
  strncpy(reinterpret_cast<char*>(write_buffer), overwrite_msg, SD::BLOCKSIZE);

  strncpy(filename, "file00", MAX_FILENAME);

  bool overwrite_success = (fs_write(filename, reinterpret_cast<char*>(write_buffer), SD::BLOCKSIZE) == 0);
  testsResult("Overwrite First File", overwrite_success);

  // Read back `file00` to verify overwrite.
  for (size_t i = 0; i < SD::BLOCKSIZE; i++) {
    read_buffer[i] = 0;
  }

  int overwrite_read = fs_read(filename, reinterpret_cast<char*>(read_buffer));
  bool overwrite_check = (overwrite_read > 0) && compareBuffers(write_buffer, read_buffer, SD::BLOCKSIZE);
  testsResult("Verify Overwritten File", overwrite_check);

  // === MULTI-BLOCK TESTS (Currently Commented Out) ===
  // The following tests were intended to verify multi-block read/write functionality.
  // These tests are currently **disabled** until BFS supports multi-block operations.
  //
  // The idea is to:
  // - Write and read a file larger than a single block.
  // - Verify that the data spans multiple blocks correctly.
  //
  // However, BFS is currently **only** handling single-block writes and reads,
  // so running these tests will cause failures.
  //
  // When multi-block functionality is added, these tests should be re-enabled.
  
  // // === TEST 5: Multi-Block Write and Read ===
  // uint32_t num_blocks = 4;  // Change this if you want larger tests.
  // uint32_t total_size = num_blocks * SD::BLOCKSIZE;
  
  // uint8_t* multi_write_buffer = new uint8_t[total_size];
  // uint8_t* multi_read_buffer = new uint8_t[total_size];

  // // Fill multi-block buffer with a pattern (ASCII letters cycling).
  // for (size_t i = 0; i < total_size; i++) {
  //   multi_write_buffer[i] = 'A' + (i % 26);
  // }

  // strncpy(filename, "bigfile", MAX_FILENAME);

  // bool multi_write_success = (fs_create(filename, total_size) == 0) &&
  //                            (fs_write(filename, reinterpret_cast<char*>(multi_write_buffer), total_size) == 0);

  // testsResult("Multi-Block Write", multi_write_success);

  // // Clear buffer before reading.
  // for (size_t i = 0; i < total_size; i++) {
  //   multi_read_buffer[i] = 0;
  // }

  // int multi_read_response = fs_read(filename, reinterpret_cast<char*>(multi_read_buffer));
  // bool multi_read_success = (multi_read_response > 0) && compareBuffers(multi_write_buffer, multi_read_buffer, total_size);

  // testsResult("Multi-Block Read and Verify", multi_read_success);

  // TODO: when delete is fixed, remove out the objects that go out of scope
}

#endif  // BFS_TESTS_H
