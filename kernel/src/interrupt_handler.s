/* Citations
  AArch64 Exception Model
*/ 

.balign 2048
.globl el1_vector_table
el1_vector_table:
  b synchronous_handler_   // Current EL with SP0
  .balign 128
  b irq_handler_
  .balign 128
  b fiq_handler_
  .balign 128
  b serror_handler_
  .balign 128
  b synchronous_handler_   // Current EL with SPX
  .balign 128
  b irq_handler_
  .balign 128
  b fiq_handler_
  .balign 128
  b serror_handler_
  .balign 128
  b synchronous_handler_   // Lower EL with Aarch64
  .balign 128
  b irq_handler_
  .balign 128
  b fiq_handler_
  .balign 128
  b serror_handler_
  .balign 128
  b synchronous_handler_   // Lower EL with Aarch32
  .balign 128
  b irq_handler_
  .balign 128
  b fiq_handler_
  .balign 128
  b serror_handler_

// TODO: Save state and work with exceptions from each level

.extern serror_handler
serror_handler_:
  bl irq_handler
  eret

.extern fiq_handler
fiq_handler_:
  bl fiq_handler
  eret

.extern irq_handler
irq_handler_:
  // Save State
  sub sp, sp, #248
  stp x0, x1, [sp, #0xe8]
  stp x2, x3, [sp, #0xd8]
  stp x4, x5, [sp, #0xc8]
  stp x6, x7, [sp, #0xb8]
  stp x8, x9, [sp, #0xa8]
  stp x10, x11, [sp, #0x98]
  stp x12, x13, [sp, #0x88]
  stp x14, x15, [sp, #0x78]
  stp x16, x17, [sp, #0x68]
  stp x18, x19, [sp, #0x58]
  stp x20, x21, [sp, #0x48]
  stp x22, x23, [sp, #0x38]
  stp x24, x25, [sp, #0x28]
  stp x26, x27, [sp, #0x18]
  stp x28, x29, [sp, #0x8]
  str x30, [sp]
  mrs x0, SPSR_EL1
  mrs x0, SP_EL1

  // Call Handler
  bl irq_handler

  // Return State
  ldp x0, x1, [sp, #0xe8]
  ldp x2, x3, [sp, #0xd8]
  ldp x4, x5, [sp, #0xc8]
  ldp x6, x7, [sp, #0xb8]
  ldp x8, x9, [sp, #0xa8]
  ldp x10, x11, [sp, #0x98]
  ldp x12, x13, [sp, #0x88]
  ldp x14, x15, [sp, #0x78]
  ldp x16, x17, [sp, #0x68]
  ldp x18, x19, [sp, #0x58]
  ldp x20, x21, [sp, #0x48]
  ldp x22, x23, [sp, #0x38]
  ldp x24, x25, [sp, #0x28]
  ldp x26, x27, [sp, #0x18]
  ldp x28, x29, [sp, #0x8]
  ldr x30, [sp]
  add sp, sp, #248
  eret

.extern synchronous_handler
synchronous_handler_:
  sub sp, sp, #248                    // Save State
  stp x0, x1, [sp, #0x0]
  stp x2, x3, [sp, #0x10]
  stp x4, x5, [sp, #0x20]
  stp x6, x7, [sp, #0x30]
  stp x8, x9, [sp, #0x40]
  stp x10, x11, [sp, #0x50]
  stp x12, x13, [sp, #0x60]
  stp x14, x15, [sp, #0x70]
  stp x16, x17, [sp, #0x80]
  stp x18, x19, [sp, #0x90]
  stp x20, x21, [sp, #0xa0]
  stp x22, x23, [sp, #0xb0]
  stp x24, x25, [sp, #0xc0]
  stp x26, x27, [sp, #0xd0]
  stp x28, x29, [sp, #0xe0]
  str x30, [sp, #0xf0]

  bl synchronous_handler              // Call Handler

  ldp x0, x1, [sp, #0x0]              // Return State
  ldp x2, x3, [sp, #0x10]
  ldp x4, x5, [sp, #0x20]
  ldp x6, x7, [sp, #0x30]
  ldp x8, x9, [sp, #0x40]
  ldp x10, x11, [sp, #0x50]
  ldp x12, x13, [sp, #0x60]
  ldp x14, x15, [sp, #0x70]
  ldp x16, x17, [sp, #0x80]
  ldp x18, x19, [sp, #0x90]
  ldp x20, x21, [sp, #0xa0]           
  ldp x22, x23, [sp, #0xb0]
  ldp x24, x25, [sp, #0xc0]
  ldp x26, x27, [sp, #0xd0]
  ldp x28, x29, [sp, #0xe0]
  ldr x30, [sp, #0xf0]
  add sp, sp, #248

  eret