ENTRY(_start)

SECTIONS
{
  /* put userspace at 4 MiB to leave the first pages unmapped */
  . = 0x0000000000400000;

  .text   : { *(.text*) }
  .rodata : { *(.rodata*) *(.rodata.*) }

  .data   : { *(.data*) }
  .bss    : { *(.bss*) *(COMMON) }

  PROVIDE(__bss_start = ADDR(.bss));
  PROVIDE(__bss_end__ = ADDR(.bss) + SIZEOF(.bss));
  PROVIDE(_end = .);
  PROVIDE(end  = .);
}
