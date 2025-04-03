/* Citations
  AArch64 Exception Model
*/ 

.extern serror_handler
.globl serror_handler_
serror_handler_:
  bl irq_handler
  eret

.extern fiq_handler
.globl fiq_handler_
fiq_handler_:
  bl fiq_handler
  eret

.extern irq_handler
.globl irq_handler_
irq_handler_:
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
  
  mov x0, sp

  mov x19, sp
  and x19, x19, #0xF
  sub sp, sp, x19

  bl irq_handler                      // Call Handler

  add sp, sp, x19

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

.extern cpu0_interrupt_stack
.extern synchronous_handler
.globl synchronous_handler_
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

  mov x0, sp

  mov x19, sp
  and x19, x19, #0xF
  sub sp, sp, x19

  bl synchronous_handler              // Call Handler

  add sp, sp, x19

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