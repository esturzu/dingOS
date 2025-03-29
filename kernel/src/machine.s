.globl get_CurrentEL
get_CurrentEL:
  mrs x0, CurrentEL
  ret

.globl get_ESR_EL1
get_ESR_EL1:
  mrs x0, ESR_EL1
  ret

.globl get_FAR_EL1
get_FAR_EL1:
  mrs x0, FAR_EL1
  ret

.globl set_VBAR_EL1
set_VBAR_EL1:
  msr VBAR_EL1, x0
  ret

