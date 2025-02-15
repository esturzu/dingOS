#include "system_timer.h"
#include "interrupts.h"
#include "printf.h"
#include "machine.h"

uint8_t SystemTimer::get_status()
{
  volatile uint32_t* control_register =
    (volatile uint32_t*)(system_timer_base_address + control_offset);

  return *control_register;
}

void SystemTimer::clear_compare(uint8_t n)
{
  volatile uint32_t* control_register =
    (volatile uint32_t*)(system_timer_base_address + control_offset);

  *control_register = (1 << n);
}

uint64_t SystemTimer::get_free_running_counter_value()
{
  volatile uint32_t* lower_32_register =
    (volatile uint32_t*)(system_timer_base_address + lower_32_offset);

  uint64_t running_counter_value = *lower_32_register;

  volatile uint32_t* higher_32_register =
    (volatile uint32_t*)(system_timer_base_address + higher_32_offset);

  running_counter_value = (uint64_t) higher_32_register << 32;

  return running_counter_value;
}

uint32_t SystemTimer::get_lower_running_counter_value()
{
  volatile uint32_t* lower_32_register =
    (volatile uint32_t*)(system_timer_base_address + lower_32_offset);

  return *lower_32_register;
}

uint32_t SystemTimer::get_compare_register(uint8_t n)
{
  if (n == 0)
  {
    volatile uint32_t* compare_0_register =
      (volatile uint32_t*)(system_timer_base_address + compare_0_offset);
    return *compare_0_register;
  }
  else if (n == 1)
  {
    volatile uint32_t* compare_1_register =
      (volatile uint32_t*)(system_timer_base_address + compare_1_offset);
    return *compare_1_register;
  }
  else if (n == 2)
  {
    volatile uint32_t* compare_2_register =
      (volatile uint32_t*)(system_timer_base_address + compare_2_offset);
    return *compare_2_register;
  }
  else 
  {
    volatile uint32_t* compare_3_register =
      (volatile uint32_t*)(system_timer_base_address + compare_3_offset);
    return *compare_3_register;
  }
}

void SystemTimer::set_compare_register(uint8_t n, uint32_t value)
{
  if (n == 0)
  {
    volatile uint32_t* compare_0_register =
      (volatile uint32_t*)(system_timer_base_address + compare_0_offset);
    *compare_0_register = value;
  }
  else if (n == 1)
  {
    volatile uint32_t* compare_1_register =
      (volatile uint32_t*)(system_timer_base_address + compare_1_offset);
    *compare_1_register = value;
  }
  else if (n == 2)
  {
    volatile uint32_t* compare_2_register =
      (volatile uint32_t*)(system_timer_base_address + compare_2_offset);
    *compare_2_register = value;
  }
  else 
  {
    volatile uint32_t* compare_3_register =
      (volatile uint32_t*)(system_timer_base_address + compare_3_offset);
    *compare_3_register = value;
  }
}

void SystemTimer::setup_timer(uint8_t n)
{
  set_compare_register(0, 25000);
  set_VBAR_EL1(&el1_vector_table);
  Interrupts::Enable_IRQ(n);

  while(true)
  {
    // Debug::printf("Time %u\n", get_lower_running_counter_value());
  }
}