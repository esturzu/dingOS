#include "stdint.h"

class ARMTimer
{
public:
  static constexpr uint64_t system_timer_base_address = 0xffff000040000000;

  static constexpr uint64_t control_offset = 0x00;

  static constexpr uint64_t core_timer_prescaler = 0x08;

  static constexpr uint64_t core_timer_access_ls = 0x1c;
  static constexpr uint64_t core_timer_access_ms = 0x20;

  static constexpr uint64_t core_0_interrupt_control = 0x40;
  static constexpr uint64_t core_1_interrupt_control = 0x44;
  static constexpr uint64_t core_2_interrupt_control = 0x48;
  static constexpr uint64_t core_3_interrupt_control = 0x4c;

  static void setup_timer();
};