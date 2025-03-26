.globl set_SPSR_EL1
set_SPSR_EL1:
  msr SPSR_EL1, x0
  ret

.globl set_ELR_EL1
set_ELR_EL1:
  msr ELR_EL1, x0
  ret

.globl set_SP_EL0
set_SP_EL0:
  msr SP_EL0, x0
  ret

.globl exception_return
exception_return:
  eret
