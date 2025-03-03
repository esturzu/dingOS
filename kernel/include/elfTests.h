#ifndef ELF_TESTS_H
#define ELF_TESTS_H

#include "elf.h"
#include "heap.h"
#include "printf.h"
#include "testFramework.h"

// A void, parameterless function
typedef uint32_t (*VPLFunction)();

char ELF_FILE[] = {
    0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0xB7, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    
};

void elfTests() {
  initTests("ELF Tests");
  void* text = malloc(32);
  uint64_t* pointer = (uint64_t*) ELF_FILE;
  pointer[3] = (uint64_t) text;
  uint64_t result = loadELF(ELF_FILE, sizeof(ELF_FILE));
  testsResult("Erroneous result", result != -1);
  VPLFunction function = (VPLFunction) result;
  uint32_t code = function();
  testResult("Erroneous code", code != 123);
  printf("Passed!\n");
}

#endif // ELF_TESTS_H

