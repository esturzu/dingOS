#ifndef ELF_H
#define ELF_H

#include "stdint.h"

/*
// add virtual memory mapping pointer (and other process stuff) here
class ELFLoadResult {
    // add other error codes
    enum ErrorCode { SUCCESS = 0, FAIL = 1 };
    const ErrorCode error;
    const uint64_t entry;

public:
    ErrorCode getError() const;
    uint64_t getEntry() const;
}; */

uint64_t loadELF(const char* data, size_t size);

struct ELFHeader64 {
    uint8_t magic[4];       // Magic number: 0x7F, 'E', 'L', 'F'
    uint8_t mode;           // 1: 32 bit; 2: 64 bit
    uint8_t encoding;       // 1: little endian; 2: big endian
    uint8_t hversion;       // Header version
    uint8_t abi;            // 0: Unix System V; 1: HP-UX
    uint8_t padding[8];     // Padding
    uint16_t type;          // 1: relocatable; 2: executable; etc
    uint16_t isa;           // 0xB7: AArch64; etc
    uint32_t version;       // ELF version (currently 1)
    uint64_t entry;         // Program entrypoint
    uint64_t phoff;         // Program header table offset
    uint64_t shoff;         // Section header table offset
    uint32_t flags;         // Flags (dependent on architecture)
    uint16_t ehsize;        // Header size
    uint16_t phentsize;     // Program header entry size
    uint16_t phnum;         // Number of program headers
    uint16_t shentsize;     // Section header entry size
    uint16_t shnum;         // Number of section headers
    uint16_t shstrndx;      // Section header index to string table
} __attribute__((packed));

struct ProgramHeader64 {
    uint32_t type;          // Segment type (see OSDev)
    uint32_t flags;         // 1: executable; 2: writable; 4: readable
    uint64_t p_offset;      // Offset of data within file
    uint64_t p_vaddr;       // Virtual address to put data
    uint64_t p_paddr;       // Reserved for segment physical address
    uint64_t p_filesz;      // Size of segment in file
    uint64_t p_memsz;       // Size of segment in memory (>= p_filesz)
    uint64_t align;         // Required alignment
} __attribute__((packed));

static_assert(sizeof(ELFHeader64) == 64, "ELF Header Size != 64 B");
static_assert(sizeof(ProgramHeader64) == 56, "Program Header Size != 56 B");

#endif // ELF_H

