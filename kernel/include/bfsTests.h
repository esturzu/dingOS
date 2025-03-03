#ifndef BFS_TESTS_H
#define BFS_TESTS_H

#include "bfs.h"
#include "printf.h"
#include "testFramework.h"

// Helper function to compare buffers manually
bool compareBuffers(const uint8_t* a, const uint8_t* b, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

// BFS tests
void bfsTests() {
  initTests("BFS Tests");

  fs_init();  // Initialize BFS

  // Buffers for writing and reading
  uint8_t* write_buffer = new uint8_t[SD::BLOCKSIZE];
  uint8_t* read_buffer = new uint8_t[SD::BLOCKSIZE]();

  // Fill write buffer with a pattern (A-Z cycle)
  for (size_t i = 0; i < SD::BLOCKSIZE - 1; i++) {
    write_buffer[i] = 'A' + (i % 26);
  }
  write_buffer[SD::BLOCKSIZE - 1] = '\0';

  // === TEST 1: Create and Fill File System ===
  char filename[MAX_FILENAME];
  int files_created = 0;
  bool create_success = true;

  for (int i = 0; i < MAX_FILES; i++) {
    strncpy(filename, "file", MAX_FILENAME);
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);
    filename[6] = '\0';

    if (fs_create(filename, SD::BLOCKSIZE) == 0) {
      files_created++;
    } else {
      create_success = false;
      break;
    }
  }

  testsResult("Create and Fill File System", create_success);

  // === TEST 2: Write to All Files ===
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
  bool read_success = true;
  for (int i = 0; i < files_created; i++) {
    filename[4] = '0' + (i / 10);
    filename[5] = '0' + (i % 10);

    // Clear read buffer manually
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
  const char* overwrite_msg = "Overwritten!";
  for (size_t i = 0; i < SD::BLOCKSIZE; i++) {
    write_buffer[i] = 0;
  }
  strncpy(reinterpret_cast<char*>(write_buffer), overwrite_msg, SD::BLOCKSIZE);

  strncpy(filename, "file00", MAX_FILENAME);

  bool overwrite_success = (fs_write(filename, reinterpret_cast<char*>(write_buffer), SD::BLOCKSIZE) == 0);
  testsResult("Overwrite First File", overwrite_success);

  // Read and check overwrite
  for (size_t i = 0; i < SD::BLOCKSIZE; i++) {
    read_buffer[i] = 0;
  }

  int overwrite_read = fs_read(filename, reinterpret_cast<char*>(read_buffer));
  bool overwrite_check = (overwrite_read > 0) && compareBuffers(write_buffer, read_buffer, SD::BLOCKSIZE);
  testsResult("Verify Overwritten File", overwrite_check);

  // Cleanup memory
}

#endif  // BFS_TESTS_H
