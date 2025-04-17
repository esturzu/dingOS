.section .text
.global save_context
.global restore_context

// void save_context(TCB* tcb)
save_context:
    mov x9, sp     // Load TCB pointer into x9
    str x9, [x0, #16]  // Save SP to tcb->savedSP (offset 8 bytes from tcb base)

    // Save General-Purpose Registers (Callee-Saved)
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x29, x30, [sp, #-16]!

    ret

// void restore_context(TCB* tcb)
restore_context:
    ldr x9, [x0, #16]  // Restore SP from tcb->savedSP
    mov sp, x9

    // Restore General-Purpose Registers
    ldp x29, x30, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16

    ret
