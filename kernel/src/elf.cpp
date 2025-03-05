#include "elf.h"
#include "printf.h"

#define UNEG1 ((uint64_t) -1)

uint64_t _error(const char* message) {
    printf("Error while loading ELF: %s\n", message);
    return UNEG1;
}

uint64_t _assertions() {
    // Verifies that the system is using little-endianness
    uint16_t value = 0x0102;
    uint8_t firstByte = *((uint8_t*) &value);
    uint8_t secondByte = *(((uint8_t*) &value) + 1);
    return firstByte == 0x02 && secondByte == 0x01 ? 0 :
        _error("invalid system endianness");
}

uint64_t loadELF(const char* data, uint64_t size) {
    if (_assertions() == UNEG1) return UNEG1;

    const ELFHeader64* header = (const ELFHeader64*) data;
    constexpr uint64_t ehsz = sizeof(ELFHeader64);
    constexpr uint64_t phsz = sizeof(ProgramHeader64);
    if (size < ehsz)               return _error("invalid file size");
    if (header->magic[0] != 0x7F)  return _error("magic[0] mismatch");
    if (header->magic[1] != 'E')   return _error("magic[1] mismatch");
    if (header->magic[2] != 'L')   return _error("magic[2] mismatch");
    if (header->magic[3] != 'F')   return _error("magic[3] mismatch");
    if (header->mode != 2)         return _error("not 64 bit mode");
    if (header->encoding != 1)     return _error("not little endian");
    if (header->isa != 0xB7)       return _error("not aarch64");
    if (header->ehsize != ehsz)    return _error("ELF header wrong size");
    if (header->phentsize != phsz) return _error("program header wrong size");
    if (header->shnum != 0)        return _error("sections not supported");

    uint64_t phoff = header->phoff;
    uint16_t phnum = header->phnum;
    uint64_t phendoff = phoff + phsz * (uint64_t) phnum;
    if (phoff > phendoff || phendoff > size) return _error("wrong PH offset");

    const ProgramHeader64* ph = (const ProgramHeader64*) (data + phoff);
    const ProgramHeader64* phend = (const ProgramHeader64*) (data + phendoff);
    for (; ph < phend; ph++) {
        uint32_t type = ph->type;
        uint64_t offset = ph->p_offset;
        uint64_t filesz = ph->p_filesz;
        uint64_t memsz = ph->p_memsz;
        uint64_t end = offset + filesz;

        if (type > 1) return _error("unsupported program header type");
        if (memsz < filesz) return _error("memsz < filesz");
        if (offset > end || end > size) return _error("invalid PH offset");
        if (ph->type == 0) continue;

        const char* dataLoc = (const char*) (data + offset);
        char* vmemLoc = (char*) ph->p_vaddr;
        char* fileEnd = vmemLoc + filesz;
        char* memEnd = vmemLoc + memsz;
        while (vmemLoc < fileEnd) *(vmemLoc++) = *(dataLoc++);
        while (vmemLoc < memEnd) *(vmemLoc++) = 0;
    }

    return header->entry;
}

