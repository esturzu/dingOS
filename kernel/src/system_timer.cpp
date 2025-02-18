#include "system_timer.h"

#include "interrupts.h"
#include "machine.h"
#include "printf.h"

volatile uint64_t current_time;

uint8_t SystemTimer::get_status() {
  volatile uint32_t* control_register =
      (volatile uint32_t*)(system_timer_base_address + control_offset);

  return *control_register;
}

void SystemTimer::clear_compare(uint8_t n) {
  volatile uint32_t* control_register =
      (volatile uint32_t*)(system_timer_base_address + control_offset);

  *control_register = (1 << n);
}

uint64_t SystemTimer::get_free_running_counter_value() {
  volatile uint32_t* lower_32_register =
      (volatile uint32_t*)(system_timer_base_address + lower_32_offset);

  uint64_t running_counter_value = *lower_32_register;

  volatile uint32_t* higher_32_register =
      (volatile uint32_t*)(system_timer_base_address + higher_32_offset);

  running_counter_value |= ((uint64_t)higher_32_register) << 32;

  return running_counter_value;
}

uint32_t SystemTimer::get_lower_running_counter_value() {
  volatile uint32_t* lower_32_register =
      (volatile uint32_t*)(system_timer_base_address + lower_32_offset);

  return *lower_32_register;
}

uint32_t SystemTimer::get_compare_register(uint8_t n) {
  volatile uint32_t* compare_register =
      (volatile uint32_t*)(system_timer_base_address + compare_0_offset +
                           4 * n);  // 4 bytes per register
  return *compare_register;
}

void SystemTimer::set_compare_register(uint8_t n, uint32_t value) {
  volatile uint32_t* compare_register =
      (volatile uint32_t*)(system_timer_base_address + compare_0_offset +
                           4 * n);  // 4 bytes per register
  *compare_register = value;
}

/**
 * @brief Sets up the timer on compare register 0 to trigger an interrupt every second
 * 
 * @param IRQ_num The IRQ number to set this timer to
 */
void SystemTimer::setup_timer(uint8_t IRQ_num) {
  current_time = 0;
  set_compare_register(0, 1000000);
  set_VBAR_EL1(&el1_vector_table);
  Interrupts::Enable_IRQ(IRQ_num);
}