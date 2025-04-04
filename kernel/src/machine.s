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

.globl get_CNTP_CTL_EL0
get_CNTP_CTL_EL0:
  mrs x0, CNTP_CTL_EL0
  ret

.globl get_CNTP_CVAL_EL0
get_CNTP_CVAL_EL0:
  mrs x0, CNTP_CVAL_EL0
  ret

.globl get_CNTP_TVAL_EL0
get_CNTP_TVAL_EL0:
  mrs x0, CNTP_TVAL_EL0
  ret

.globl set_VBAR_EL1
set_VBAR_EL1:
  msr VBAR_EL1, x0
  ret

.globl set_CNTKCTL_EL1
set_CNTKCTL_EL1:
  msr CNTKCTL_EL1, x0
  ret

.globl set_CNTP_CTL_EL0
set_CNTP_CTL_EL0:
  msr CNTP_CTL_EL0, x0
  ret

.globl set_CNTP_CVAL_EL0
set_CNTP_CVAL_EL0:
  msr CNTP_CVAL_EL0, x0
  ret

.globl set_CNTP_TVAL_EL0
set_CNTP_TVAL_EL0:
  msr CNTP_TVAL_EL0, x0
  ret

.globl set_DAIFClr_all
set_DAIFClr_all:
  msr DAIFClr, #7
  ret

.extern 
.globl set_stack_pointer
set_stack_pointer:
  mov sp, x0
  ret
