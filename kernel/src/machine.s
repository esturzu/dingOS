.globl get_CurrentEL
get_CurrentEL:
  mrs x0, CurrentEL
  ret

.globl set_VBAR_EL1
set_VBAR_EL1:
  msr VBAR_EL1, x0
  ret
