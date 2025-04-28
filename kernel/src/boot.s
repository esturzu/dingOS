.section ".text.boot"

.extern stack0_top
.extern dtb_location
.extern setup_kernel_vm
.extern enable_kernel_vm
.globl _start
_start:
  adrp x1, :pg_hi21:dtb_location
  eor x0, x0, #0xFFFF000000000000
  str x0, [x1, :lo12:dtb_location]
  mov x0, #0x80000000 // Allow for 64 bit mode boot
  msr HCR_EL2, x0
  ldr x0, =kernelMain // Setup return address after returning el2 -> el1
  msr ELR_EL2, x0
  ldr x0, =stack0_top // Setup stack after returning el2 -> el1
  eor x0, x0, #0xFFFF000000000000
  ldr x0, [x0]
  msr SP_EL1, x0
  mov x0, #0x5        // Set eret to return to el1 with el1 stack ("EL1 with SP_EL1 (EL1h)")
  msr SPSR_EL2, x0
  mov x0, #0x100000   // Allows el1 to execute floating point unit without exception
  msr CPACR_EL1, x0
  bl setup_kernel_vm
  bl enable_kernel_vm
  adrp x0, :pg_hi21:core_0_el1_vector_table   // Enable Interrupts
  add x0, x0, :lo12:core_0_el1_vector_table
  orr x0, x0, 0xFFFF000000000000
  msr VBAR_EL1, x0
  eret

.extern stack1_top
.globl _start_core1
_start_core1:
  mov x0, #0x80000000 // Allow for 64 bit mode boot
  msr HCR_EL2, x0
  ldr x0, =initCore1  // Setup return address after returning el2 -> el1
  msr ELR_EL2, x0
  ldr x0, =stack1_top // Setup stack after returning el2 -> el1
  eor x0, x0, #0xFFFF000000000000
  ldr x0, [x0]
  msr SP_EL1, x0 
  mov x0, #0x5        // Set eret to return to el1 with el1 stack ("EL1 with SP_EL1 (EL1h)")
  msr SPSR_EL2, x0
  mov x0, #0x100000   // Allows el1 to execute floating point unit without exception
  msr CPACR_EL1, x0
  bl enable_kernel_vm
  adrp x0, :pg_hi21:core_1_el1_vector_table   // Enable Interrupts
  add x0, x0, :lo12:core_1_el1_vector_table
  orr x0, x0, 0xFFFF000000000000
  msr VBAR_EL1, x0
  eret

.extern stack2_top
.globl _start_core2
_start_core2:
  mov x0, #0x80000000 // Allow for 64 bit mode boot
  msr HCR_EL2, x0
  ldr x0, =initCore2  // Setup return address after returning el2 -> el1
  msr ELR_EL2, x0
  ldr x0, =stack2_top // Setup stack after returning el2 -> el1
  eor x0, x0, #0xFFFF000000000000
  ldr x0, [x0]
  msr SP_EL1, x0 
  mov x0, #0x5        // Set eret to return to el1 with el1 stack ("EL1 with SP_EL1 (EL1h)")
  msr SPSR_EL2, x0
  mov x0, #0x100000   // Allows el1 to execute floating point unit without exception
  msr CPACR_EL1, x0
  bl enable_kernel_vm
  adrp x0, :pg_hi21:core_2_el1_vector_table   // Enable Interrupts
  add x0, x0, :lo12:core_2_el1_vector_table
  orr x0, x0, 0xFFFF000000000000
  msr VBAR_EL1, x0
  eret

.extern stack3_top
.globl _start_core3
_start_core3:
  mov x0, #0x80000000 // Allow for 64 bit mode boot
  msr HCR_EL2, x0
  ldr x0, =initCore3  // Setup return address after returning el2 -> el1
  msr ELR_EL2, x0
  ldr x0, =stack3_top // Setup stack after returning el2 -> el1
  eor x0, x0, #0xFFFF000000000000
  ldr x0, [x0]
  msr SP_EL1, x0 
  mov x0, #0x5        // Set eret to return to el1 with el1 stack ("EL1 with SP_EL1 (EL1h)")
  msr SPSR_EL2, x0
  mov x0, #0x100000   // Allows el1 to execute floating point unit without exception
  msr CPACR_EL1, x0
  bl enable_kernel_vm
  adrp x0, :pg_hi21:core_3_el1_vector_table   // Enable Interrupts
  add x0, x0, :lo12:core_3_el1_vector_table
  orr x0, x0, 0xFFFF000000000000
  msr VBAR_EL1, x0
  eret
