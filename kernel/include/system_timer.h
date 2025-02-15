// Citations
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "stdint.h"

class SystemTimer {

public:

  static constexpr uint64_t system_timer_base_address = 0x3f003000;

  static constexpr uint64_t control_offset = 0x00;
  static constexpr uint64_t lower_32_offset = 0x04;
  static constexpr uint64_t higher_32_offset = 0x08;
  static constexpr uint64_t compare_0_offset = 0x0c;
  static constexpr uint64_t compare_1_offset = 0x10;
  static constexpr uint64_t compare_2_offset = 0x14;
  static constexpr uint64_t compare_3_offset = 0x18;

  static uint8_t get_status();
  static void clear_compare(uint8_t n);
  static uint64_t get_free_running_counter_value();
  static uint32_t get_lower_running_counter_value();
  static uint32_t get_compare_register(uint8_t n);
  static void set_compare_register(uint8_t n, uint32_t value);

  static void setup_timer(uint8_t n);
};