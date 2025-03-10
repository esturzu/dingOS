#include "elf.h"
#include "printf.h"

#define INVALID(start, end, size) ((start) > (end) || (end) > (size))

ELFLoader::Result ELFLoader::load(const char* data, uint64_t size) {
  const ELFHeader64* header = (const ELFHeader64*) data;
  constexpr uint64_t ehsz = sizeof(ELFHeader64);
  constexpr uint64_t phsz = sizeof(ProgramHeader64);
  if (size < ehsz)               return Result(INVALID_FILE_SIZE);
  if (header->magic[0] != 0x7F)  return Result(INVALID_ELF_SIGNATURE);
  if (header->magic[1] != 'E')   return Result(INVALID_ELF_SIGNATURE);
  if (header->magic[2] != 'L')   return Result(INVALID_ELF_SIGNATURE);
  if (header->magic[3] != 'F')   return Result(INVALID_ELF_SIGNATURE);
  if (header->mode != 2)         return Result(UNSUPPORTED_BIT_MODE);
  if (header->encoding != 1)     return Result(UNSUPPORTED_ENDIANNESS);
  if (header->type != 2)         return Result(UNSUPPORTED_ELF_TYPE);
  if (header->isa != 0xB7)       return Result(UNSUPPORTED_ARCH_ISA);
  if (header->ehsize != ehsz)    return Result(INVALID_ELF_HEADER_SIZE);
  if (header->phentsize != phsz) return Result(INVALID_PROGRAM_HEADER_SIZE);
  if (header->shnum != 0)        return Result(UNSUPPORTED_SECTIONS);

  uint64_t phoff = header->phoff;
  uint16_t phnum = header->phnum;
  uint64_t phendoff = phoff + phsz * (uint64_t) phnum;
  if (phnum == 0) return Result(header->entry);
  if (INVALID(phoff, phendoff, size)) {
    return Result(INVALID_PROGRAM_HEADER_OFFSET);
  }

  const ProgramHeader64* ph = (const ProgramHeader64*) (data + phoff);
  const ProgramHeader64* phend = (const ProgramHeader64*) (data + phendoff);
  for (; ph < phend; ph++) {
    uint32_t type = ph->type;
    if (type == 0) continue;
    if (type != 1) return Result(UNSUPPORTED_PROGRAM_HEADER_TYPE);

    uint64_t off = ph->p_offset;
    uint64_t filesz = ph->p_filesz;
    uint64_t memsz = ph->p_memsz;
    uint64_t end = off + filesz;
    if (memsz < filesz) return Result(INVALID_MEM_SIZE);
    if (INVALID(off, end, size)) {
      return Result(INVALID_PROGRAM_HEADER_DATA_OFFSET);
    }

    const char* dataLoc = (const char*) (data + off);
    char* vmemLoc = (char*) ph->p_vaddr;
    char* fileEnd = vmemLoc + filesz;
    char* memEnd = vmemLoc + memsz;
    while (vmemLoc < fileEnd) *(vmemLoc++) = *(dataLoc++);
    while (vmemLoc < memEnd) *(vmemLoc++) = 0;
  }

  return Result(header->entry);
}

