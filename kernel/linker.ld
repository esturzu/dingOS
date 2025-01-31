/*
To reunderstand what each part means, visit https://wiki.osdev.org/Linker_Scripts
*/
ENTRY(_start)

SECTIONS
{
    . = 0x80000;
    .text :
    {
        KEEP(*(.text.boot))
        *(.text)
    }
    .rodata : {*(.rodata)}
    .data : {*(.data)}
    .bss : {*(.bss)}
    _end = .;
}