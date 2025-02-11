.section ".text.boot"

.extern stack0_top
.globl _start
_start:
  mov x0, #0x80000000 // Allow for 64 bit mode boot
  msr HCR_EL2, x0
  ldr x0, =kernelMain // Setup return address after returning el2 -> el1
  msr ELR_EL2, x0
  ldr x0, =stack0_top // Setup stack after returning el2 -> el1
  ldr x0, [x0]
  msr SP_EL1, x0 
  mov x0, #0x5 // Set eret to return to el1 with el1 stack
  msr SPSR_EL2, x0
  mov x0, #0x5 // Set eret to return to el1 with el1 stack
  msr SPSR_EL2, x0
  mov x0, #0x100000 // Allows el1 to execute floating point unit without exception
  msr CPACR_EL1, x0
  eret

.extern stack1_top
.globl _start_core1
_start_core1:
  ldr x5, =stack1_top
  ldr x5, [x5]
  mov sp, x5
  bl initCore1

.extern stack2_top
.globl _start_core2
_start_core2:
  ldr x5, =stack2_top
  ldr x5, [x5]
  mov sp, x5
  bl initCore2

.extern stack3_top
.globl _start_core3
_start_core3:
  ldr x5, =stack3_top
  ldr x5, [x5]
  mov sp, x5
  bl initCore3
