#ifndef SD_TESTS_H
#define SD_TESTS_H

#include "printf.h"
#include "sd.h"
#include "testFramework.h"

void printBuffer(uint8_t* buffer, uint32_t size, uint32_t width) {
  for (uint32_t i = 0; i < size; i++) {
    if (i % SD::BLOCKSIZE == 0) {
      debug_printf("Block\n");
    }
    debug_printf("%02x", buffer[i]);
    // only add a space if end of a 8byte chunk
    if ((i + 1) % 4 == 0) {
      debug_printf(" ");
    }
    if ((i + 1) % (width * 4) == 0) {
      debug_printf("\n");
    }
  }
}

bool compareBuffers(uint8_t* expectedBuffer, uint8_t* readBuffer,
                    uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (expectedBuffer[i] != readBuffer[i]) {
      debug_printf("↓↓↓Buffers do not match at index %u: 0x%02x != 0x%02x\n", i,
                   expectedBuffer[i], readBuffer[i]);
      return false;
    }
  }
  return true;
}

void clearBuffer(uint8_t* buffer, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    buffer[i] = 0;
  }
}

void sdTests() {
  initTests("SD Tests");

  uint32_t startBlock, startAddress, blocks, res = SD::SUCCESS, width = 4,
                                             compareRes;
  uint8_t* readBuffer;
  uint8_t* expectedBuffer;

  // Test 1: Read from inital disk (3 blocks)
  startBlock = 0;
  startAddress = startBlock * SD::BLOCKSIZE;
  blocks = 3;
  readBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  // create an expected buffer that when read in uint32_t litte endian will be
  // 3, 7, 11, ...
  expectedBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  for (uint32_t i = 0; i < blocks * SD::BLOCKSIZE; i += 4) {
    *(uint32_t*)(expectedBuffer + i) = i + 3 + startAddress;
  }

  res = SD::read(startBlock, blocks, readBuffer);
  compareRes =
      compareBuffers(expectedBuffer, readBuffer, blocks * SD::BLOCKSIZE);
  testsResult("Read from inital disk (3 blocks)",
              res == blocks * SD::BLOCKSIZE && compareRes);
  // delete[] readBuffer;
  // delete[] expectedBuffer;

  // Test 2: Read from inital disk (1 block)
  startBlock = 3;
  startAddress = startBlock * SD::BLOCKSIZE;
  blocks = 1;
  readBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  expectedBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  for (uint32_t i = 0; i < blocks * SD::BLOCKSIZE; i += 4) {
    *(uint32_t*)(expectedBuffer + i) = i + 3 + startAddress;
  }

  res = SD::read(startBlock, blocks, readBuffer);
  compareRes =
      compareBuffers(expectedBuffer, readBuffer, blocks * SD::BLOCKSIZE);
  testsResult("Read from inital disk (1 block)",
              res == blocks * SD::BLOCKSIZE && compareRes);

  // delete[] readBuffer;
  // delete[] expectedBuffer;

  // Test 3: Write 3 and read back
  startBlock = 20;
  startAddress = startBlock * SD::BLOCKSIZE;
  blocks = 3;
  readBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  // creating a buffer that when read in uint8_t will be 0, 1, 2, ... 255, 0, 1,
  // ...
  expectedBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  for (int i = 0; i < blocks * SD::BLOCKSIZE; i++) {
    expectedBuffer[i] = (i + startAddress) % 0x100;
  }
  // delete[] readBuffer;
  // delete[] expectedBuffer;

  // writing the buffer to the disk
  res = SD::write(startBlock, blocks, expectedBuffer);

  // Read back the actually written blocks
  res = SD::read(startBlock, blocks, readBuffer);
  compareRes =
      compareBuffers(expectedBuffer, readBuffer, blocks * SD::BLOCKSIZE);
  testsResult("Write 3 and read back",
              res == blocks * SD::BLOCKSIZE && compareRes);
  clearBuffer(readBuffer, blocks * SD::BLOCKSIZE);

  // Test 4: Write 1 and read back
  startBlock = 25;
  startAddress = startBlock * SD::BLOCKSIZE;
  blocks = 1;
  readBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  // creating a buffer that when read in uint8_t will be 0, 0, 2, ... 255, 0, 0,
  // 2, 0 , 4 ...
  expectedBuffer = new uint8_t[blocks * SD::BLOCKSIZE];
  for (int i = 0; i < blocks * SD::BLOCKSIZE; i++) {
    if ((i + startAddress) % 2 == 1) {
      expectedBuffer[i] = 0;
    } else {
      expectedBuffer[i] = (i + startAddress) % 0x100;
    }
  }

  // writing the buffer to the disk
  res = SD::write(startBlock, blocks, expectedBuffer);

  // Read back the actually written blocks
  res = SD::read(startBlock, blocks, readBuffer);
  compareRes =
      compareBuffers(expectedBuffer, readBuffer, blocks * SD::BLOCKSIZE);
  testsResult("Write 1 and read back",
              res == blocks * SD::BLOCKSIZE && compareRes);
  clearBuffer(readBuffer, blocks * SD::BLOCKSIZE);

  // delete[] readBuffer;
  // delete[] expectedBuffer;
}

#endif