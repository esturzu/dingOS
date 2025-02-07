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
    _heap_end = ORIGIN(ram) + LENGTH(ram) - 0x100000; /* Reserve 1MB for peripherals and stack */

    _stack_top = ORIGIN(ram) + LENGTH(ram);
    _stack_bottom = _stack_top - 0x100000; /* 1MB stack region */
}