ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0xFFFF000000080000, LENGTH = 0x3E000000 /* 1GB RAM */
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
        . = ALIGN(8);
        _init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        _init_array_end = .;
    } > ram

    .fini_array :
    {
        . = ALIGN(8);
        _fini_array_start = .;
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array))
        _fini_array_end = .;
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
    _heap_end = _heap_start + 0x10000000; /* 256 MB for heap space */

    
    _frame_start = _heap_end;
    _frame_end = _heap_end + 0x20000000; /* 512 MB for page space */
}