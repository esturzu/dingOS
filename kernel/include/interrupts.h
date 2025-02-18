// Citations
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
// https://developer.arm.com/documentation/dui0203/j/handling-processor-exceptions/armv6-and-earlier--armv7-a-and-armv7-r-profiles/interrupt-handlers

#include "stdint.h"

class Interrupts {
 public:
  static constexpr uint64_t interrupt_base_address = 0x3f00b000;

  static constexpr uint64_t IRQ_basic_pending_offset = 0x200;
  static constexpr uint64_t IRQ_pending_1_offset = 0x204;
  static constexpr uint64_t IRQ_pending_2_offset = 0x208;
  static constexpr uint64_t FIQ_control_offset = 0x20c;
  static constexpr uint64_t Enable_IRQ_1_offset = 0x210;
  static constexpr uint64_t Enable_IRQ_2_offset = 0x214;
  static constexpr uint64_t Enable_basic_IRQ_offset = 0x218;
  static constexpr uint64_t Disable_IRQ_1_offset = 0x21c;
  static constexpr uint64_t Disable_IRQ_2_offset = 0x220;
  static constexpr uint64_t Disable_basic_IRQ_offset = 0x224;

  static uint32_t get_basic_pending_register();
  static uint32_t get_IRQ_pending_1_register();
  static uint32_t get_IRQ_pending_2_register();
  static void set_fiq_interrupt(uint8_t interrupt_source);
  static void disable_fiq_interrupt();
  static void Enable_IRQ(uint8_t IRQ_num);
  static void Enable_Base(uint8_t Offset);
  static void Disable_IRQ(uint8_t IRQ_num);
  static void Disable_All_IRQ();
  static void Disable_All_Base(uint8_t Offset);
};