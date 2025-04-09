#include "elf.h"

#define INVALID(start, end, size) ((start) > (end) || (end) > (size))

ELFLoader::Result ELFLoader::load(const char* data, uint64_t size,
                                  Process* process) {
  // Verifies that the input data is non-null
  if (data == NULL)    return INVALID_NULL_DATA;
  if (process == NULL) return INVALID_NULL_PROCESS;

  // Verifies that the system is little-endian
  constexpr uint64_t endianValueU64 = 0x0806060504030201;
  const uint8_t* endianPtrU8 = (uint8_t*) &endianValueU64;
  for (uint32_t i = 0; i < 8; i++) {
    if (endianPtrU8[i] != i + 1) return UNSUPPORTED_SYSTEM_ENDIANNESS;
  }

  // Verifies that the ELF header is valid and supported
  const ELFHeader64* header = (const ELFHeader64*) data;
  constexpr uint64_t ehsz = sizeof(ELFHeader64);
  constexpr uint64_t phsz = sizeof(ProgramHeader64);
  if (size < ehsz)               return INVALID_FILE_SIZE;
  if (header->magic[0] != 0x7F)  return INVALID_ELF_SIGNATURE;
  if (header->magic[1] != 'E')   return INVALID_ELF_SIGNATURE;
  if (header->magic[2] != 'L')   return INVALID_ELF_SIGNATURE;
  if (header->magic[3] != 'F')   return INVALID_ELF_SIGNATURE;
  if (header->mode != 2)         return UNSUPPORTED_BIT_MODE;
  if (header->encoding != 1)     return UNSUPPORTED_ENDIANNESS;
  if (header->type != 2)         return UNSUPPORTED_ELF_TYPE;
  if (header->isa != 0xB7)       return UNSUPPORTED_ARCH_ISA;
  if (header->ehsize != ehsz)    return INVALID_ELF_HEADER_SIZE;
  if (header->phentsize != phsz) return INVALID_PROGRAM_HEADER_SIZE;
  if (header->shnum != 0)        return UNSUPPORTED_SECTIONS;

  // Verifies that the program header table is valid
  uint64_t phoff = header->phoff;
  uint16_t phnum = header->phnum;
  uint64_t phendoff = phoff + phsz * (uint64_t) phnum;
  if (INVALID(phoff, phendoff, size)) return INVALID_PROGRAM_HEADER_OFFSET;

  // Verifies that each program header is valid and supported
  const ProgramHeader64* phstart = (const ProgramHeader64*) (data + phoff);
  const ProgramHeader64* phend = (const ProgramHeader64*) (data + phendoff);
  for (const ProgramHeader64* ph = phstart; ph < phend; ph++) {
    uint32_t type = ph->type;
    if (type == 0) continue;
    if (type != 1) return UNSUPPORTED_PROGRAM_HEADER_TYPE;

    uint64_t off = ph->p_offset;
    uint64_t filesz = ph->p_filesz;
    uint64_t memsz = ph->p_memsz;
    uint64_t end = off + filesz;
    if (memsz < filesz) return INVALID_MEM_SIZE;
    if (INVALID(off, end, size)) return INVALID_PROGRAM_HEADER_DATA_OFFSET;
  }

  // Iterates through each program header and loads the corresponding data
  for (const ProgramHeader64* ph = phstart; ph < phend; ph++) {
    uint32_t type = ph->type;
    if (type == 0) continue;

    uint64_t off = ph->p_offset;
    uint64_t filesz = ph->p_filesz;
    uint64_t memsz = ph->p_memsz;
    uint64_t end = off + filesz;

    const char* dataLoc = (const char*) (data + off);
    char* vmemLoc = (char*) ph->p_vaddr;
    char* fileEnd = vmemLoc + filesz;
    char* memEnd = vmemLoc + memsz;

    process->map_range((uint64_t) vmemLoc, (uint64_t) memEnd);
    while (vmemLoc < fileEnd) *(vmemLoc++) = *(dataLoc++);
    while (vmemLoc < memEnd) *(vmemLoc++) = 0;
  }

  // Sets the entry point and returns success
  process->set_entry_point(header->entry);
  return SUCCESS;
}

