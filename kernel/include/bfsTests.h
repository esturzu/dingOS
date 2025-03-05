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

  debug_printf("BFS Tests: Initializing filesystem...\n");
  fs_init();

  uint8_t* write_buffer = new uint8_t[BLOCK_SIZE * 4];  // Changed! Now supports multi-block tests! Changed!
  uint8_t* read_buffer = new uint8_t[BLOCK_SIZE * 4]();

  // Fill the write buffer with a repeating pattern of A-Z.
  for (size_t i = 0; i < BLOCK_SIZE * 4; i++) {
    write_buffer[i] = 'A' + (i % 26);
  }

  // === TEST 1: Create and Fill File System ===
  char filename[MAX_FILENAME];
  int files_created = 0;
  bool create_success = true;

  debug_printf("BFS Tests: Creating files...\n");
  for (int i = 0; i < 7; i++) {
    strncpy(filename, "file", MAX_FILENAME);
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);
    filename[6] = '\0';

    if (fs_create(filename, BLOCK_SIZE) == 0) {
      files_created++;
    } else {
      create_success = false;
      debug_printf("BFS Tests: Failed to create file %s\n", filename);
      break;
    }
  }

  testsResult("Create and Fill File System", create_success);

  // === TEST 2: Write to All Files ===
  bool write_success = true;
  debug_printf("BFS Tests: Writing to all files...\n");

  for (int i = 0; i < files_created; i++) {
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);

    int write_result = fs_write(filename, reinterpret_cast<char*>(write_buffer), BLOCK_SIZE);
    debug_printf("BFS Tests: Writing to %s, result=%d\n", filename, write_result);
    if (write_result != BLOCK_SIZE) {
      write_success = false;
      break;
    }
  }

  testsResult("Write to All Files", write_success);

  // === TEST 3: Read Back and Verify Data ===
  bool read_success = true;
  debug_printf("BFS Tests: Reading and verifying files...\n");

  for (int i = 0; i < files_created; i++) {
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);

    for (size_t j = 0; j < BLOCK_SIZE; j++) {
      read_buffer[j] = 0;
    }

    int read_result = fs_read(filename, reinterpret_cast<char*>(read_buffer), BLOCK_SIZE);
    debug_printf("BFS Tests: Read from %s, result=%d\n", filename, read_result);

    if (read_result != BLOCK_SIZE || !compareBuffers(write_buffer, read_buffer, BLOCK_SIZE)) {
      read_success = false;
      break;
    }
  }

  testsResult("Read and Verify Data Integrity", read_success);

  // === TEST 4: Multi-Block Write ===
  debug_printf("BFS Tests: Multi-Block Write Test starting...\n");

  strncpy(filename, "bigfile", MAX_FILENAME);
  bool multi_write_success = (fs_create(filename, BLOCK_SIZE * 4) == 0) &&
                             (fs_write(filename, reinterpret_cast<char*>(write_buffer), BLOCK_SIZE * 4) == BLOCK_SIZE * 4);

  debug_printf("BFS Tests: Multi-Block Write result = %d\n", multi_write_success);
  testsResult("Multi-Block Write", multi_write_success);

  // === TEST 5: Multi-Block Read ===
  debug_printf("BFS Tests: Multi-Block Read Test starting...\n");

  for (size_t i = 0; i < BLOCK_SIZE * 4; i++) {
    read_buffer[i] = 0;
  }

  int multi_read_result = fs_read(filename, reinterpret_cast<char*>(read_buffer), BLOCK_SIZE * 4);
  bool multi_read_success = (multi_read_result == BLOCK_SIZE * 4) && compareBuffers(write_buffer, read_buffer, BLOCK_SIZE * 4);

  debug_printf("BFS Tests: Multi-Block Read result=%d\n", multi_read_result);
  testsResult("Multi-Block Read and Verify", multi_read_success);
}

#endif  // BFS_TESTS_H
