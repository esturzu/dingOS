.section ".text.boot"

.globl _start
_start:
  ldr x5, =_start
  mov sp, x5
  bl kernelMain
