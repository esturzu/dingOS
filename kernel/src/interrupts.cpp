// Citations
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
// https://developer.arm.com/documentation/ddi0601/2025-03/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "interrupts.h"

#include "cores.h"
#include "local_timer.h"
#include "event_loop.h"
#include "machine.h"
#include "printf.h"
#include "process.h"
#include "system_call.h"
#include "system_timer.h"

using Debug::panic;

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
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_2_offset);
    *Disable_IRQ_2_register = (1 << (IRQ_num - 32));
  }
}

void Interrupts::Disable_All_IRQ() {
  volatile uint32_t* Disable_IRQ_1_register =
      (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
  *Disable_IRQ_1_register = 0xFFFFFFFF;

  volatile uint32_t* Disable_IRQ_2_register =
      (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_2_offset);
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
  bool local_timer_trigger = LocalTimer::check_interrupt();

  uint8_t current_core = SMP::whichCore();

  if (local_timer_trigger && activeProcess[current_core] != nullptr)
  {
    // Need to Preempt Process
    if (!(get_SPSR_EL1() & 0xF))
    {
      // EL0
      debug_printf("Preempt Process In EL0\n");

      Process* current_process = activeProcess[current_core];

      current_process->save_state(saved_state);

      activeProcess[current_core] = nullptr;

      __asm__ volatile("dmb sy" ::: "memory");

      schedule_event([current_process](){
        current_process->run();
      });

      set_DAIFClr_all();

      event_loop();
    } 
    else
    {
      // EL1
      // todo: Handle Preemption in Kernel
    }
  }

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
        panic("EL1 sync: Unknown reason   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000001:
      {
        panic("EL1 sync: Trapped WF* instruction   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000011:
      {
        panic("EL1 sync: Trapped MCR (A32)   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000100:
      {
        panic("EL1 sync: Trapped MCRR (A32)   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000101:
      {
        panic("EL1 sync: Trapped MCR (T32)   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000110:
      {
        panic("EL1 sync: Trapped LDC/STC access   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b000111:
      {
        panic("EL1 sync: Access to SME, SVE, Advanced SIMD, or floating-point   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b001010:
      {
        panic("EL1 sync: Not covered error class   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b001100:
      {
        panic("EL1 sync: Trapped MRRC (A32)   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b001101:
      {
        panic("EL1 sync: Branch-Target exception   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b001110:
      {
        panic("EL1 sync: Illegal Execution state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b010001:
      {
        panic("EL1 sync: SVC Instruction Execution in AArch32 state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b010100:
      {
        panic("EL1 sync: Trapped MSRR, MRRS, or System Instruction Execution   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b010101:
      {
        const uint16_t syscall_type = error_syndrome_register & 0xFFFF;
        system_call_handler(syscall_type, saved_state);
        return;
      }
      break;
    case 0b011000:
      {
        panic("EL1 sync: Trapped MSRR, MRRS, or System Instruction Execution   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b011001:
      {
        panic("EL1 sync: Access to SVE functionality   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b011011:
      {
        panic("EL1 sync: Exception from an access to a TSTART instruction   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b011100:
      {
        panic("EL1 sync: Exception from a PAC Fail   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b011101:
      {
        panic("EL1 sync: Access to SME functionality   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b100000:
      {
        panic("EL1 sync: Instruction Abort from a lower exception level   ESR=0x%lx FAR=0x%lx", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b100001:
      {
        panic("EL1 sync: Instruction Abort taken without a change in exception level   ESR=0x%lx FAR=0x%lx", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b100010:
      {
        panic("EL1 sync: PC alignment fault exception   ESR=0x%lx FAR=0x%lx", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b100100:
      {
        panic("EL1 sync: Data Abort exception from a lower exception level   ESR=0x%lx FAR=0x%lx ", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b100101:
      {
        panic("EL1 sync: Data Abort exception taken without a change in exception level   ESR=0x%lx FAR=0x%lx ", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b100110:
      {
        panic("EL1 sync: SP alignment fault exception   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b100111:
      {
        panic("EL1 sync: Memory Operation Exception   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b101000:
      {
        panic("EL1 sync: Trapped floating-point exception taken from AArch32 state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b101100:
      {
        panic("EL1 sync: Trapped floating-point exception taken from AArch64 state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b101101:
      {
        panic("EL1 sync: GCS exception   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b101111:
      {
        panic("EL1 sync: SError exception   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b110000:
      {
        panic("EL1 sync: Breakpoint exception from a lower exception level   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b110001:
      {
        panic("EL1 sync: Breakpoint exception taken without a change in exception level   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b110010:
      {
        panic("EL1 sync: Software Step exception from a lower exception level   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b110011:
      {
        panic("EL1 sync: Software step exception take without a change in exception level   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b110100:
      {
        panic("EL1 sync: Watchpoint exception from a lower exception level   ESR=0x%lx FAR=0x%lx", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b110101:
      {
        panic("EL1 sync: Watchpoint exception taken without a change in exception level   ESR=0x%lx FAR=0x%lx", error_syndrome_register, get_FAR_EL1());
      }
      break;
    case 0b111000:
      {
        panic("EL1 sync: BKPT instruction execution in AArch32 state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    case 0b111100:
      {
        panic("EL1 sync: BKPT instruction execution in AArch64 state   ESR=0x%lx", error_syndrome_register);
      }
      break;
    default:
      {
        panic("EL1 sync: Unknown ESR_EL1 Value   ESR=0x%lx", error_syndrome_register);
      }
  }
}
