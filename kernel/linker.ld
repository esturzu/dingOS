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

    /* Define heap and stack regions */
    _heap_start = ALIGN(8);
    _heap_end = _heap_start + 0x20000000; /* 128KB for heap space */
}