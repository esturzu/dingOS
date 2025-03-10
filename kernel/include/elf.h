#ifndef ELF_H
#define ELF_H

#include "stdint.h"

namespace ELFLoader {
  enum ErrorCode {
    // Successful code(s): 0x00 through 0x3F inclusive: if this code is
    // received, then the ELF loaded correctly into memory, and it is ready
    SUCCESS = 0,

    // Unsupported code(s): 0x40 through 0x7F inclusive: these codes signify
    // that a particular ELF configuration may be valid, but that it is not
    // currently supported by this ELF loader (for example, dynamic loading)
    UNSUPPORTED_BIT_MODE            = 0x40,
    UNSUPPORTED_ENDIANNESS          = 0x41,
    UNSUPPORTED_ELF_TYPE            = 0x42,
    UNSUPPORTED_ARCH_ISA            = 0x43,
    UNSUPPORTED_SECTIONS            = 0x44,
    UNSUPPORTED_PROGRAM_HEADER_TYPE = 0x45,

    // Invalid code(s): 0x80 through 0xBF inclusive: these codes signify
    // that an ELF file has an invalid configuration
    INVALID_FILE_SIZE                  = 0x80,
    INVALID_ELF_SIGNATURE              = 0x81,
    INVALID_ELF_HEADER_SIZE            = 0x82,
    INVALID_PROGRAM_HEADER_SIZE        = 0x83,
    INVALID_PROGRAM_HEADER_OFFSET      = 0x84,
    INVALID_MEM_SIZE                   = 0x85,
    INVALID_PROGRAM_HEADER_DATA_OFFSET = 0x86
  };

  class Result {
  public:
    Result(ErrorCode code) : code(code), entry(0) {}
    Result(uint64_t entry) : code(SUCCESS), entry(entry) {}
    bool success() const { return (int) code < 0x40; }
    bool unsupported() const { return 0x40 <= (int) code && (int) code < 0x80; }
    bool invalid() const { return 0x80 <= (int) code && (int) code < 0xC0; }
    ErrorCode getError() const { return code; }
    uint64_t getEntry() const { return entry; }

  private:
    const ErrorCode code;
    const uint64_t entry;
  };

  Result load(const char* data, size_t size);

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
};

#endif // ELF_H

