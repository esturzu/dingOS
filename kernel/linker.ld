ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x80000, LENGTH = 0x3E000000 /* 1GB RAM */
}

SECTIONS
{
    .text :
    {
        KEEP(*(.text.boot))
        *(.text*)
    } > ram

    .init_array :
    {
        crti.o(.init_array)
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .init_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .init_array))
        crtn.o(.init_array)
    } > ram

    .fini_array :
    {
        crti.o(.fini_array)
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .fini_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .fini_array))
        crtn.o(.fini_array)
    } > ram

    .rodata :
    {
        *(.rodata*)
    } > ram

    .data :
    {
        *(.data*)
    } > ram

    .bss (NOLOAD) :
    {
        *(.bss*)
        _end = .;
    } > ram

    /* Define heap and page region */
    _heap_start = ALIGN(8);
    _heap_end = _heap_start + 0x20000000; /* 512 MB for heap space */

    
    _frame_start = _heap_end;
    _frame_end = _heap_end + 0x10000000; /* 256 MB for page space */
}