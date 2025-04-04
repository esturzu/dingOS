// Citations
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
// https://developer.arm.com/documentation/ddi0601/2025-03/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "interrupts.h"

#include "local_timer.h"
#include "event_loop.h"
#include "machine.h"
#include "printf.h"
#include "system_call.h"
#include "system_timer.h"

#define INTERRUPT_STACK_SIZE 4096

uint8_t cpu0_interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));
uint8_t cpu1_interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));
uint8_t cpu2_interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));
uint8_t cpu3_interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));

uint32_t Interrupts::get_basic_pending_register() {
  volatile uint32_t* IRQ_basic_pending_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_basic_pending_offset);
  return *IRQ_basic_pending_register;
}

uint32_t Interrupts::get_IRQ_pending_1_register() {
  volatile uint32_t* IRQ_pending_1_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_pending_1_offset);
  return *IRQ_pending_1_register;
}

uint32_t Interrupts::get_IRQ_pending_2_register() {
  volatile uint32_t* IRQ_pending_2_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_pending_2_offset);
  return *IRQ_pending_2_register;
}

void Interrupts::set_fiq_interrupt(uint8_t interrupt_source) {
  volatile uint32_t* FIQ_control_register =
      (volatile uint32_t*)(interrupt_base_address + FIQ_control_offset);
  *FIQ_control_register = interrupt_source & 0b10000000;
}

void Interrupts::disable_fiq_interrupt() {
  volatile uint32_t* FIQ_control_register =
      (volatile uint32_t*)(interrupt_base_address + FIQ_control_offset);
  *FIQ_control_register = 0;
}

void Interrupts::Enable_IRQ(uint8_t IRQ_num) {
  if (IRQ_num < 32) {
    volatile uint32_t* Enable_IRQ_1_register =
        (volatile uint32_t*)(interrupt_base_address + Enable_IRQ_1_offset);
    *Enable_IRQ_1_register = (1 << IRQ_num);
  } else {
    volatile uint32_t* Enable_IRQ_2_register =
        (volatile uint32_t*)(interrupt_base_address + Enable_IRQ_2_offset);
    *Enable_IRQ_2_register = (1 << (IRQ_num - 32));
  }
}

void Interrupts::Enable_Base(uint8_t Offset) {
  volatile uint32_t* Enable_basic_IRQ_register =
      (volatile uint32_t*)(interrupt_base_address + Enable_basic_IRQ_offset);
  *Enable_basic_IRQ_register = (1 << Offset);
}

void Interrupts::Disable_IRQ(uint8_t IRQ_num) {
  if (IRQ_num < 32) {
    volatile uint32_t* Disable_IRQ_1_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
    *Disable_IRQ_1_register = (1 << IRQ_num);
  } else {
    volatile uint32_t* Disable_IRQ_2_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
    *Disable_IRQ_2_register = (1 << (IRQ_num - 32));
  }
}

void Interrupts::Disable_All_IRQ() {
  volatile uint32_t* Disable_IRQ_1_register =
      (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
  *Disable_IRQ_1_register = 0xFFFFFFFF;

  volatile uint32_t* Disable_IRQ_2_register =
      (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
  *Disable_IRQ_2_register = 0xFFFFFFFF;
}

void Interrupts::Disable_All_Base(uint8_t Offset) {
  volatile uint32_t* Disable_basic_IRQ_register =
      (volatile uint32_t*)(interrupt_base_address + Disable_basic_IRQ_offset);
  *Disable_basic_IRQ_register = 0xFFFFFFFF;
}

extern "C" void serror_handler()
{

}

extern "C" void fiq_handler()
{
 
}

extern "C" void irq_handler(uint64_t* saved_state)
{
  LocalTimer::check_interrupt();

  uint32_t irq_pending_1 = Interrupts::get_IRQ_pending_1_register();

  // System Timer 1 Interrupt
  if (irq_pending_1 & 0b1) {
    current_time += 1;
    uint32_t current_lower = SystemTimer::get_lower_running_counter_value();
    SystemTimer::set_compare_register(0, current_lower + 1000000);
    SystemTimer::clear_compare(0);
  }
}

extern "C" void synchronous_handler(uint64_t* saved_state)
{
  uint64_t error_syndrome_register = get_ESR_EL1();
  uint64_t exception_class = (error_syndrome_register >> 26) & 0x3F;
  bool trapped_32_bit_instruction = error_syndrome_register & (1 << 25);

  switch (exception_class)
  {
    case 0b000000:
      {
        printf("ESR_EL1 Unknown Reason\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000001:
      {
        printf("Trapped WF* instruction\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000011:
      {
        printf("Trapped MCR\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000100:
      {
        printf("Trapped MCRR\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000101:
      {
        printf("Trapped MCR\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000110:
      {
        printf("Trapped LDC or STC access\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b000111:
      {
        printf("Access to SME, SVE, Advanced SIMD, or floating-point\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b001010:
      {
        printf("Not covered error class\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b001100:
      {
        printf("Trapped MRRC\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b001101:
      {
        printf("Branch Target Exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b001110:
      {
        printf("Illegal Execution State\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b010001:
      {
        printf("SVC Instruction Execution in AArch32 state\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b010100:
      {
        printf("Trapped MSRR, MRRS, or System Instruction Execution\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b010101:
      {
        printf("SVC instruction execution in AArch64 state\n");
        uint16_t syscall_type = error_syndrome_register & 0xFFFF;
        system_call_handler(syscall_type, saved_state);
        while(1){} // Replace with PANIC
      }
      break;
    case 0b011000:
      {
        printf("Trapped MSRR, MRRS, or System Instruction Execution\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b011001:
      {
        printf("Access to SVE functionality\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b011011:
      {
        printf("Exception from an access to a TSTART instruction\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b011100:
      {
        printf("Exception from a PAC Fail\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b011101:
      {
        printf("Access to SME functionality\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100000:
      {
        printf("Instruction Abort from a lower exception level %lx %lx\n", get_ESR_EL1(), get_FAR_EL1());
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100001:
      {
        printf("Instruction Abort taken without a change in exception level %lx %lx\n", get_ESR_EL1(), get_FAR_EL1());
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100010:
      {
        printf("PC alignment fault exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100100:
      {
        printf("Data Abort exception from a lower exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100101:
      {
        printf("Data Abort exception taken without a change in exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100110:
      {
        printf("SP alignment fault exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b100111:
      {
        printf("Memory Operation Exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b101000:
      {
        printf("Trapped floating-point exception taken from AArch32 state\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b101100:
      {
        printf("Trapped floating-point exception taken from AArch64 state\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b101101:
      {
        printf("GCS exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b101111:
      {
        printf("SError exception\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110000:
      {
        printf("Breakpoint exception from a lower exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110001:
      {
        printf("Breakpoint exception taken without a change in exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110010:
      {
        printf("Software Step exception from a lower exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110011:
      {
        printf("Software step exception take without a change in exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110100:
      {
        printf("Watchpoint exception from a lower exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b110101:
      {
        printf("Watchpoint exception taken without a change in exception level\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b111000:
      {
        printf("BKPT instruction execution in AArch32 state\n");
        while(1){} // Replace with PANIC
      }
      break;
    case 0b111100:
      {
        printf("BKPT instruction execution in AArch64 state\n");
        while(1){} // Replace with PANIC
      }
      break;
    default:
      {
        printf("Unknown ESR_EL1 Value\n");
        while(1){} // Replace with PANIC
      }
  }

  while(1){}
}