/* Citations
  Armv8 A-profile architecture Reference Manual
*/ 

.balign 4096
.globl kernel_vm_base_tbl
kernel_vm_base_tbl:
  .word 0x0

.balign 4096
.globl kernel_vm_level_1_table
kernel_vm_level_1_table:
  .word 0x0

.balign 4096
.globl setup_kernel_vm
setup_kernel_vm:
  adrp x0, :pg_hi21:kernel_vm_base_tbl   // Get Address for VMM Tables
  adrp x1, :pg_hi21:kernel_vm_level_1_table
  movz x2, #0x0003, lsl #0    // Setup Attribute BitMask
  movk x2, #0x0000, lsl #48
  orr x2, x1, x2
  str x2, [x0]
  movz x2, #0x0401, lsl #0  // Setup Attribute BitMask
  movk x2, #0x0000, lsl #16
  movk x2, #0x0000, lsl #48
  str x2, [x1]
  ret

.globl enable_kernel_vm
enable_kernel_vm:
  adrp x0, :pg_hi21:kernel_vm_base_tbl    // Setup TTBR1_EL1
  msr TTBR1_EL1, x0
  movz x0, #0x0000, lsl #0    // Setup TCR_EL1
  movk x0, #0x8010, lsl #16
  movk x0, #0x00c5, lsl #32
  movk x0, #0x0080, lsl #48
  msr TCR_EL1, x0
  ldr x0, =0xFF   // Setup MAIR_EL1
  msr MAIR_EL1, x0
  isb
  mrs x0, SCTLR_EL1   // Setup SCTLR_EL1
  orr x0, x0, #0x1
  msr SCTLR_EL1, x0
  isb
  ret

.globl get_TTBR0_EL1
get_TTBR0_EL1:
  mrs x0, TTBR0_EL1
  ret

.globl get_TTBR1_EL1
get_TTBR1_EL1:
  mrs x0, TTBR1_EL1
  ret

.globl get_MAIR_EL1
get_MAIR_EL1:
  mrs x0, MAIR_EL1
  ret

.globl get_SCTLR_EL1
get_SCTLR_EL1:
  mrs x0, SCTLR_EL1
  ret

.globl get_TCR_EL1
get_TCR_EL1:
  mrs x0, TCR_EL1
  ret

.globl get_ID_AA64MMFR1_EL1
get_ID_AA64MMFR1_EL1:
  mrs x0, ID_AA64MMFR1_EL1
  ret

.globl get_ID_AA64MMFR2_EL1
get_ID_AA64MMFR2_EL1:
  mrs x0, ID_AA64MMFR2_EL1
  ret

.globl set_TTBR0_EL1
set_TTBR0_EL1:
  msr TTBR0_EL1, x0
  ret

.globl set_TTBR1_EL1
set_TTBR1_EL1:
  msr TTBR1_EL1, x0
  ret

.globl set_MAIR_EL1
set_MAIR_EL1:
  msr MAIR_EL1, x0
  ret

.globl set_SCTLR_EL1
set_SCTLR_EL1:
  msr SCTLR_EL1, x0
  ret

.globl set_TCR_EL1
set_TCR_EL1:
  msr TCR_EL1, x0
  ret

.globl tlb_invalidate_all
  tlbi alle1
  ret
