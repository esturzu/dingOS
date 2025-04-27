    .global _start
_start:
    bl  __libc_init_array      // newlib global ctors
    bl  main
    mov x0, x0                 // main’s return in x0
    mov x8, #93                // "exit” svc number
    svc #0
1:  wfi                        // should never return
    b  1b
