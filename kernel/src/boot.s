.global _start

.section .text.boot
_start:
    // Set the stack pointer
    ldr x0, =_stack_top
    mov sp, x0

    // Zero out BSS section
    ldr x1, =_end
    ldr x2, =_heap_start
zero_bss:
    cmp x1, x2
    b.eq bss_zero_done
    str xzr, [x1], #8
    b zero_bss
bss_zero_done:

    // Call kernel main
    bl kernelMain

    // Infinite loop to stop execution
halt:
    wfi
    b halt
    