#ifndef ELF_TESTS_H
#define ELF_TESTS_H

#include "elf.h"
#include "bfs.h"
#include "testFramework.h"

typedef uint32_t (*SampleProcessFunction)();
const size_t NUM_INSTRUCTIONS = 2;
uint32_t PROGRAM_LOAD_LOC[NUM_INSTRUCTIONS];

struct ELFFile {
  ELFLoader::ELFHeader64 header;
  ELFLoader::ProgramHeader64 programHeader;
  uint32_t code[NUM_INSTRUCTIONS];
} __attribute__((packed));

const ELFFile ELF_FILE = (ELFFile) {
  // ELF Header
  (ELFLoader::ELFHeader64) {
    { 0x7F, 'E', 'L', 'F' },       // Magic ELF Signature
    0x02, 0x01, 0x01, 0x00,        // Mode, Encoding, HVersion, ABI
    { },                           // Padding
    0x0002, 0x00B7, 0x0000'0001,   // Type, ISA, Version
    (uint64_t) PROGRAM_LOAD_LOC,   // Entry
    0x0000'0000'0000'0040,         // Program Header Table Offset
    0x0000'0000'0000'0000,         // Section Header Table Offset
    0x0000'0000, 0x0040, 0x0038,   // Flags, EH Size, PH Size
    0x0001, 0x0000, 0x0000, 0x0000 // PH Number, S Size, S Num, S S Idx
  },

  // Program Header
  (ELFLoader::ProgramHeader64) {
    0x0000'0001, 0x0000'0007,          // Type, Flags
    0x0000'0000'0000'0078,             // Offset
    (uint64_t) PROGRAM_LOAD_LOC,       // Virtual Address
    0x0000'0000'0000'0000,             // Physical Address
    (uint64_t) (NUM_INSTRUCTIONS * 4), // File Size
    (uint64_t) (NUM_INSTRUCTIONS * 4), // Mem Size
    0x0000'0000'0000'0004              // Align
  },

  // Machine Code (Text)
  {
    0x52800F60, // mov w0, 123
    0xD65F03C0  // ret
  }
};

/*
char ELF_FILE[] = {
    0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0xB7, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x60, 0x0F, 0x80, 0x52, 0xC0, 0x03, 0x5F, 0xD6
}; */

char READ_BUFFER[sizeof(ELF_FILE)];

bool equal(const char* a, const char* b, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void elfTests() {
  initTests("ELF Tests");
  const char* data = (const char*) &ELF_FILE;
  constexpr size_t size = sizeof(ELF_FILE);

  /*
  fs_create("sample.elf", 0);
  fs_write("sample.elf", data, size);
  fs_read("sample.elf", READ_BUFFER);
  testsResult("File Write + Read", equal(data, READ_BUFFER, size));
  */

  ELFLoader::Result result = ELFLoader::load(data, size);
  bool success = result.success();
  testsResult("ELF Load", success);

  bool validRun = false;
  if (success) {
    SampleProcessFunction function = (SampleProcessFunction) result.getEntry();
    uint32_t output = function();
    validRun = output == 123;
  }
  testsResult("Run File", validRun);
}

#endif // ELF_TESTS_H

