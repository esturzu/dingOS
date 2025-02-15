#include "interrupts.h"
#include "printf.h"
#include "uart.h"

uint32_t Interrupts::get_basic_pending_register()
{
  volatile uint32_t* IRQ_basic_pending_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_basic_pending_offset);
  return *IRQ_basic_pending_register;
}

uint32_t Interrupts::get_IRQ_pending_1_register()
{
  volatile uint32_t* IRQ_pending_1_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_pending_1_offset);
  return *IRQ_pending_1_register;
}

uint32_t Interrupts::get_IRQ_pending_2_register()
{
  volatile uint32_t* IRQ_pending_2_register =
      (volatile uint32_t*)(interrupt_base_address + IRQ_pending_2_offset);
  return *IRQ_pending_2_register;
}

void Interrupts::set_fiq_interrupt(uint8_t interrupt_source)
{
  volatile uint32_t* FIQ_control_register =
      (volatile uint32_t*)(interrupt_base_address + FIQ_control_offset);
  *FIQ_control_register = interrupt_source & 0b10000000;
}

void Interrupts::disable_fiq_interrupt()
{
  volatile uint32_t* FIQ_control_register =
      (volatile uint32_t*)(interrupt_base_address + FIQ_control_offset);
  *FIQ_control_register = 0;
}

void Interrupts::Enable_IRQ(uint8_t IRQ_num)
{
  if (IRQ_num < 32)
  {
    volatile uint32_t* Enable_IRQ_1_register =
        (volatile uint32_t*)(interrupt_base_address + Enable_IRQ_1_offset);
    *Enable_IRQ_1_register = (1 << IRQ_num);
  }
  else
  {
    volatile uint32_t* Enable_IRQ_2_register =
        (volatile uint32_t*)(interrupt_base_address + Enable_IRQ_2_offset);
    *Enable_IRQ_2_register = (1 << (IRQ_num - 32));
  }
}

void Interrupts::Enable_Base(uint8_t Offset)
{
  volatile uint32_t* Enable_basic_IRQ_register =
        (volatile uint32_t*)(interrupt_base_address + Enable_basic_IRQ_offset);
    *Enable_basic_IRQ_register = (1 << Offset);
}

void Interrupts::Disable_IRQ(uint8_t IRQ_num)
{
  if (IRQ_num < 32)
  {
    volatile uint32_t* Disable_IRQ_1_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
    *Disable_IRQ_1_register = (1 << IRQ_num);
  }
  else
  {
    volatile uint32_t* Disable_IRQ_2_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
    *Disable_IRQ_2_register = (1 << (IRQ_num - 32));
  }
}

void Interrupts::Disable_All_IRQ()
{
  volatile uint32_t* Disable_IRQ_1_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
  *Disable_IRQ_1_register = 0xFFFFFFFF;

  volatile uint32_t* Disable_IRQ_2_register =
    (volatile uint32_t*)(interrupt_base_address + Disable_IRQ_1_offset);
  *Disable_IRQ_2_register = 0xFFFFFFFF;
}

void Interrupts::Disable_All_Base(uint8_t Offset)
{
  volatile uint32_t* Disable_basic_IRQ_register =
        (volatile uint32_t*)(interrupt_base_address + Disable_basic_IRQ_offset);
    *Disable_basic_IRQ_register = 0xFFFFFFFF;
}

extern "C" void irq_handler()
{
  Debug::printf("Hello\n");
  while(1) {}
}