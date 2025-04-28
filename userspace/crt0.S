    .section .text
    .global _start
_start:
    bl  __libc_init_array    // C / C++ global ctors (newlib)
    bl  main                 // main() -> result in x0
    svc #0x00                // SYS_exit  (x0 = status)
1:  wfi
    b   1b                   // should never get here
