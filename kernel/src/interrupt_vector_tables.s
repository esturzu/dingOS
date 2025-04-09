/* Citations
  AArch64 Exception Model
*/ 

.extern serror_handler_
.extern fiq_handler_
.extern irq_handler_
.extern synchronous_handler_

.balign 2048
.globl core_0_el1_vector_table
core_0_el1_vector_table:
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


.balign 2048
.globl core_1_el1_vector_table
core_1_el1_vector_table:
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


.balign 2048
.globl core_2_el1_vector_table
core_2_el1_vector_table:
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


.balign 2048
.globl core_3_el1_vector_table
core_3_el1_vector_table:
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
