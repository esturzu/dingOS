.section ".text.boot"

.extern stack0_top
.globl _start
_start:
  ldr x5, =stack0_top
  ldr x5, [x5]
  mov sp, x5
  bl kernelMain

.extern stack1_top
.globl _start_core1
_start_core1:
  ldr x5, =stack1_top
  ldr x5, [x5]
  mov sp, x5
  bl kernelMain_core1

.extern stack2_top
.globl _start_core2
_start_core2:
  ldr x5, =stack2_top
  ldr x5, [x5]
  mov sp, x5
  bl kernelMain_core2

.extern stack3_top
.globl _start_core3
_start_core3:
  ldr x5, =stack3_top
  ldr x5, [x5]
  mov sp, x5
  bl kernelMain_core3

    