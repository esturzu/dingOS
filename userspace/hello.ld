VIRT_ADDR = 0x0000000000400000;

ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);   /* PF_R | PF_X */
  data PT_LOAD FLAGS(6);   /* PF_R | PF_W */
}

SECTIONS
{
  /* ------------- code + rodata (read-exec) ------------------ */
  . = VIRT_ADDR;
  .text : AT(ADDR(.text) - VIRT_ADDR)  /* ==> file offset 0   */
  {
    *(.text*)
    *(.rodata*) *(.rodata.*)
  } :text

  /* ------------- data + bss (read-write) -------------------- */
  . = ALIGN(0x1000);                     /* keep pages clean   */
  .data : AT(ADDR(.data) - VIRT_ADDR)
  {
    *(.data*) *(.bss*) *(COMMON)
  } :data

  /* Symbols newlib expects for _sbrk */
  PROVIDE(__bss_start = ADDR(.bss));
  PROVIDE(__bss_end__  = ADDR(.bss)+SIZEOF(.bss));
  PROVIDE(_end = .);   PROVIDE(end = .);
}
