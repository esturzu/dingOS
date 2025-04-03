#include "stdint.h"

class ARMTimer
{
public:
  static constexpr uint64_t system_timer_base_address = 0xffff000040000000;

  static constexpr uint64_t local_interrupt_routing_offset = 0x24;

  static constexpr uint64_t timer_control_status_offset = 0x34;
  static constexpr uint64_t timer_clear_reload_offset = 0x38;
  static constexpr uint64_t core_0_timer_int_control_offset = 0x40;

  static void setup_timer();
};