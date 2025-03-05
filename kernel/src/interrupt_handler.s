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
  eret
  .balign 128
  eret
  .balign 128
  b synchronous_handler_   // Current EL with SPX
  .balign 128
  b irq_handler_
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  b synchronous_handler_   // Lower EL with Aarch64
  .balign 128
  b irq_handler_
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  b synchronous_handler_   // Lower EL with Aarch32
  .balign 128
  b irq_handler_
  .balign 128
  eret
  .balign 128
  eret

// TODO: Save state and work with exceptions from each level

.extern irq_handler
irq_handler_:
  bl irq_handler
  eret

.extern synchronous_handler
synchronous_handler_:
  bl synchronous_handler
  eret