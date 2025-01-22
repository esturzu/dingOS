.section ".text.boot"

.globl _start

  .org 0x80000
_start:
  ldr x5, =_start
  mov sp, x5
  bl kernelMain
