/* Citations
  AArch64 Exception Model
*/ 

.balign 2048
.globl el1_vector_table
el1_vector_table:
  eret
  .balign 128
  bl irq_handler_
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  bl irq_handler_
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  bl irq_handler_
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  eret
  .balign 128
  bl irq_handler_
  .balign 128
  eret
  .balign 128
  eret

.extern irq_handler
irq_handler_:
  bl irq_handler
  eret
