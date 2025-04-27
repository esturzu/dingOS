#include "elf.h"

#define INVALID(start, end, size) ((start) > (end) || (end) > (size))

static inline uint64_t min_u64(uint64_t a, uint64_t b) { return a < b ? a : b; }

static inline bool buf_eq(const void* a, const void* b, uint64_t n)
{
    const uint8_t* pa = static_cast<const uint8_t*>(a);
    const uint8_t* pb = static_cast<const uint8_t*>(b);
    for (uint64_t i = 0; i < n; ++i) if (pa[i] != pb[i]) return false;
    return true;
}

static inline void buf_copy(void* dst, const void* src, uint64_t n)
{
    uint8_t* d = static_cast<uint8_t*>(dst);
    const uint8_t* s = static_cast<const uint8_t*>(src);
    for (uint64_t i = 0; i < n; ++i) d[i] = s[i];
}

static inline void buf_set(void* dst, uint8_t val, uint64_t n)
{
    uint8_t* d = static_cast<uint8_t*>(dst);
    for (uint64_t i = 0; i < n; ++i) d[i] = val;
}

/* ------------------------------------------------------------------ */
/*  Flush freshly-written code so it is executable                    */
/* ------------------------------------------------------------------ */
static inline void flush_icache_range(uint64_t va, uint64_t size)
{
    const uint64_t line = 64;                     /* A53 line length   */
    uint64_t start = va & ~(line - 1);
    uint64_t end   = (va + size + line - 1) & ~(line - 1);

    for (uint64_t addr = start; addr < end; addr += line)
        asm volatile ("dc cvau, %0" :: "r"(addr) : "memory");

    asm volatile ("dsb ish" ::: "memory");

    for (uint64_t addr = start; addr < end; addr += line)
        asm volatile ("ic ivau, %0" :: "r"(addr));

    asm volatile ("dsb ish; isb" ::: "memory");
}

ELFLoader::Result
ELFLoader::load(const char* data, uint64_t size, Process* process)
{
    if (!data)     return INVALID_NULL_DATA;
    if (!process)  return INVALID_NULL_PROCESS;

    /* endian check (little‑endian required) */
    constexpr uint64_t endian_test = 0x0807060504030201ULL;
    if (*(const uint8_t*)&endian_test != 0x01)
        return UNSUPPORTED_SYSTEM_ENDIANNESS;

    const ELFHeader64* eh = reinterpret_cast<const ELFHeader64*>(data);
    constexpr uint64_t EH_SIZE = sizeof(ELFHeader64);
    constexpr uint64_t PH_SIZE = sizeof(ProgramHeader64);

    if (size < EH_SIZE)                     return INVALID_FILE_SIZE;
    if (!buf_eq(eh->magic, "\x7F""ELF", 4)) return INVALID_ELF_SIGNATURE;
    if (eh->mode      != 2)                return UNSUPPORTED_BIT_MODE;
    if (eh->encoding  != 1)                return UNSUPPORTED_ENDIANNESS;
    if (eh->type      != 2)                return UNSUPPORTED_ELF_TYPE;
    if (eh->isa       != 0xB7)             return UNSUPPORTED_ARCH_ISA;
    if (eh->ehsize    != EH_SIZE)          return INVALID_ELF_HEADER_SIZE;
    if (eh->phentsize != PH_SIZE)          return INVALID_PROGRAM_HEADER_SIZE;

    const uint64_t ph_end_off = eh->phoff + uint64_t(eh->phnum) * PH_SIZE;
    if (ph_end_off > size)                 return INVALID_PROGRAM_HEADER_OFFSET;

    const ProgramHeader64* ph_begin =
        reinterpret_cast<const ProgramHeader64*>(data + eh->phoff);
    const ProgramHeader64* ph_end   =
        reinterpret_cast<const ProgramHeader64*>(data + ph_end_off);

    /* validate headers first */
    for (const ProgramHeader64* ph = ph_begin; ph < ph_end; ++ph)
    {
        if (ph->type == 1)   /* PT_LOAD */
        {
            if (ph->p_memsz < ph->p_filesz)
                return INVALID_MEM_SIZE;
            if (ph->p_offset + ph->p_filesz > size)
                return INVALID_PROGRAM_HEADER_DATA_OFFSET;
        }
        else if (ph->type == 0  || ph->type == 6 ||
                 ph->type == 0x6474e551 || ph->type == 0x6474e552)
        {
            continue;        /* ignored types */
        }
        else
            return UNSUPPORTED_PROGRAM_HEADER_TYPE;
    }

    /* load PT_LOAD segments */
    for (const ProgramHeader64* ph = ph_begin; ph < ph_end; ++ph)
    {
        if (ph->type != 1) continue;

        uint64_t file_off  = ph->p_offset;
        uint64_t file_size = ph->p_filesz;
        uint64_t mem_size  = ph->p_memsz;
        uint64_t vaddr     = ph->p_vaddr;

        const uint8_t* src = reinterpret_cast<const uint8_t*>(data + file_off);
        uint64_t       done = 0;

        while (done < mem_size)
        {
            uint64_t cur_va   = vaddr + done;
            uint64_t page_off = cur_va & 0xFFFULL;
            uint64_t chunk    = min_u64(0x1000 - page_off, mem_size - done);

            uint8_t* dst_page =
                static_cast<uint8_t*>(process->map_one_page(cur_va & ~0xFFFULL));
            if (!dst_page) return INVALID_MEM_SIZE;    /* out of memory */

            if (done < file_size)
            {
                uint64_t file_chunk = min_u64(chunk, file_size - done);
                buf_copy(dst_page + page_off, src, file_chunk);
                if (file_chunk < chunk)
                    buf_set(dst_page + page_off + file_chunk, 0,
                            chunk - file_chunk);
                src += file_chunk;
            }
            else
                buf_set(dst_page + page_off, 0, chunk);

            done += chunk;
        }
    }

    /* entry point & user stack */
    process->set_entry_point(eh->entry);

    constexpr uint64_t STACK_SIZE = 64 * 1024;
    constexpr uint64_t STACK_TOP  = 0x0000'0000'8000'0000ULL;
    constexpr uint64_t STACK_LOW  = STACK_TOP - STACK_SIZE;
    process->map_range(STACK_LOW, STACK_TOP);
    process->set_initial_sp(STACK_TOP);

    /* ------------------------------------------------------------------ */
    /*  Make all PT_LOAD segments executable                              */
    /* ------------------------------------------------------------------ */
    for (const ProgramHeader64 *ph = ph_begin; ph < ph_end; ++ph)
    {
        if (ph->type != 1) continue;        /* only PT_LOAD */
        flush_icache_range(ph->p_vaddr, ph->p_memsz);
    }

    return SUCCESS;
}
