/* PUBLIC ELF API
 * 
 * SAMPLE USAGE
 * 
 * extern const char* contents; // Contents of the ELF file
 * extern size_t size;          // Size of the ELF file
 * extern Process* process;     // Process pointer (see "process.h")
 *
 * ELFLoader::Result result = ELFLoader::load(contents, size, process);
 * bool successful = result.success();
 * bool unsupported = result.unsupported();
 * bool invalid = result.invalid();
 *
 * if (result) {
 *   // Loading was successful (ELFLoader::Result can be auto-casted to a
 *   // boolean; "(bool) result" is guaranteed to equal "result.success()")
 * } else if (result == ELFLoader::UNSUPPORTED_SYSTEM_ENDIANNESS) {
 *   // The system has a different endianness than expected (ELFLoader::Result
 *   // can be compared with, and casted to, an ELFLoader::ErrorCode value)
 * } else {
 *   // Some other error occurred
 * }
 * 
 * GENERAL DESCRIPTION
 * 
 * namespace ELFLoader {
 *     enum ErrorCode { ... }
 *     class Result { ... }
 *     Result load(const char* data, size_t size, Process* process);
 * }
 * 
 * To use the ELF loader, call ELFLoader::load(<contents>, <size>, <process>),
 * where <contents> is a string (as const char*) representing the data, <size>
 * is the length of the <contents> string (as size_t), and <process> is a
 * Process pointer (as Process*), where the Process class is defined in
 * "process.h". This would return an ELFLoader::Result, which just contains an
 * ELFLoader::ErrorCode.
 * 
 * Error codes have three distinct categories: successful (program loaded
 * correctly), unsupported (the ELF file appears correct so far, but contains
 * features that the loader does not implement), or invalid (the ELF loader
 * appears incorrect). If a successful code is returned, it means that all
 * applicable memory mappings, as well as the program entry point, have been
 * added to the Process*. Otherwise, if an unsuccessful code (so either
 * unsupported or invalid) is returned, then it guarantees that the input
 * Process* would not have been modified.
 *
 * Refer to the ELFLoader::ErrorCode enum for a list of possible error codes,
 * and refer to the ELFLoader::Result definition below for a list of helpful
 * functions and operator overloads.
 */

#ifndef ELF_H
#define ELF_H

#include "stdint.h"
#include "process.h"

#define IN_ANY_TYPE(start, end, value) ((start) <= (value) && (value) < (end))
#define IN_U32(start, end, value) IN_ANY_TYPE(start, end, (uint32_t) value)

/* ELFLoader Namespace */
namespace ELFLoader {
  /* Error Codes (Including Success) */
  enum ErrorCode : uint8_t {
    // Successful code(s): 0x00 through 0x3F inclusive: if this code is
    // received, then the ELF loaded correctly into memory, and it is ready
    SUCCESS = 0,

    // Unsupported code(s): 0x40 through 0x7F inclusive: these codes signify
    // that a particular ELF configuration may be valid, but that it is not
    // currently supported by this ELF loader (for example, dynamic loading)
    UNSUPPORTED_BIT_MODE             = 0x40,
    UNSUPPORTED_ENDIANNESS           = 0x41,
    UNSUPPORTED_ELF_TYPE             = 0x42,
    UNSUPPORTED_ARCH_ISA             = 0x43,
    UNSUPPORTED_SECTIONS             = 0x44,
    UNSUPPORTED_PROGRAM_HEADER_TYPE  = 0x45,
    UNSUPPORTED_SYSTEM_ENDIANNESS    = 0x46,
    UNSUPPORTED_PAGE_UNALIGNED_VADDR = 0x47,

    // Invalid code(s): 0x80 through 0xBF inclusive: these codes signify
    // that one of the inputs (such as the ELF file or the process) has an
    // invalid configuration
    INVALID_FILE_SIZE                  = 0x80,
    INVALID_ELF_SIGNATURE              = 0x81,
    INVALID_ELF_HEADER_SIZE            = 0x82,
    INVALID_PROGRAM_HEADER_SIZE        = 0x83,
    INVALID_PROGRAM_HEADER_OFFSET      = 0x84,
    INVALID_MEM_SIZE                   = 0x85,
    INVALID_PROGRAM_HEADER_DATA_OFFSET = 0x86,
    INVALID_NULL_DATA                  = 0x87,
    INVALID_NULL_PROCESS               = 0x88
  };

  /* Result Class */
  class Result {
    /* Public Functions */
    public:
      constexpr Result(ErrorCode code) : code(code) {}
      constexpr ErrorCode getErrorCode() const { return code; }
      constexpr bool success() const { return (uint32_t) code < 0x40; }
      constexpr bool unsupported() const { return IN_U32(0x40, 0x80, code); }
      constexpr bool invalid() const { return IN_U32(0x80, 0xC0, code); }
      constexpr operator ErrorCode() const { return getErrorCode(); }
      constexpr operator bool() const { return success(); }

    /* Internal Data */
    private:
      const ErrorCode code;
  };

  /* Load Function */
  Result load(const char* data, size_t size, Process* process);

  /* 64-Bit ELF Header */
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
    uint64_t entry;         // Program entry point
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

  /* 64-Bit Program Header */
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

#undef IN_ANY_TYPE
#undef IN_U32

#endif // ELF_H

