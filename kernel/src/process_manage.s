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

.globl enter_process
enter_process:
  ldr x1, [x0, #8]
  ldr x2, [x0, #16]
  ldr x3, [x0, #24]
  ldr x4, [x0, #32]
  ldr x5, [x0, #40]
  ldr x6, [x0, #48]
  ldr x7, [x0, #56]
  ldr x8, [x0, #64]
  ldr x9, [x0, #72]
  ldr x10, [x0, #80]
  ldr x11, [x0, #88]
  ldr x12, [x0, #96]
  ldr x13, [x0, #104]
  ldr x14, [x0, #112]
  ldr x15, [x0, #120]
  ldr x16, [x0, #128]
  ldr x17, [x0, #136]
  ldr x18, [x0, #144]
  ldr x19, [x0, #152]
  ldr x20, [x0, #160]
  ldr x21, [x0, #168]
  ldr x22, [x0, #176]
  ldr x23, [x0, #184]
  ldr x24, [x0, #192]
  ldr x25, [x0, #200]
  ldr x26, [x0, #208]
  ldr x27, [x0, #216]
  ldr x28, [x0, #224]
  ldr x29, [x0, #232]
  ldr x30, [x0, #240]
  ldr x0, [x0]
  eret
