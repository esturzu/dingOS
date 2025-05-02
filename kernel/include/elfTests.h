#ifndef ELF_TESTS_H
#define ELF_TESTS_H

#include "elf.h"
#include "event_loop.h"
#include "testFramework.h"

typedef uint32_t (*SampleProcessFunction)();
constexpr size_t NUM_INSTRUCTIONS = 7;
uint64_t PROGRAM_LOAD_LOC = 0xFFFF'0000'0000'0000;

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
    PROGRAM_LOAD_LOC,              // Entry
    0x0000'0000'0000'0040,         // Program Header Table Offset
    0x0000'0000'0000'0000,         // Section Header Table Offset
    0x0000'0000, 0x0040, 0x0038,   // Flags, EH Size, PH Size
    0x0001, 0x0000, 0x0000, 0x0000 // PH Number, S Size, S Num, S S Idx
  },

  // Program Header
  (ELFLoader::ProgramHeader64) {
    0x0000'0001, 0x0000'0007,          // Type, Flags
    0x0000'0000'0000'0078,             // Offset
    PROGRAM_LOAD_LOC,                  // Virtual Address
    0x0000'0000'0000'0000,             // Physical Address
    (uint64_t) (NUM_INSTRUCTIONS * 4), // File Size
    (uint64_t) (NUM_INSTRUCTIONS * 4), // Mem Size
    0x0000'0000'0000'0004              // Align
  },

  // Machine Code (Text)
  {
    0xD2800D00, // mov x0, 104 // 'h'
    0xD4000041, // svc 2
    0xD2800D20, // mov x0, 105 // 'i'
    0xD4000041, // svc 2
    0xD2800140, // mov x0, 10  // '\n'
    0xD4000041, // svc 2
    0xD4000001  // svc 0
  }
};

/*
char ELF_FILE[] = {
    // ELF Header
    0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0xB7, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Program Header
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Machine Code (Text): mov x0, #123 ; ret
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
  char* data = (char*) &ELF_FILE;
  constexpr size_t size = sizeof(ELF_FILE);

  Process* process = new Process();
  ELFLoader::Result result = ELFLoader::load(data, size, process);
  bool success = result.success();
  testsResult("ELF Load 1", success);

  if (success) {
    schedule_event([process] () { process->run(); });
    unsigned long value = 12345;
    for (long i = 0; i < 200'000'000; i++) value = value * 768 + 90;
    printf(" ELF should have printed 'hi' above\n");
    printf(" ELF random result: %lu\n", value);
  } else {
    delete process;
  }
}

#endif // ELF_TESTS_H

