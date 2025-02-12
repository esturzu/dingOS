// Citation
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "definitions.h"
#include "stdint.h"

class GPIO {
 public:
  /**
   * @brief PUD - GPIO Pin Pull-up/down
   * @details
   * 00 = Off – disable pull-up/down
   * 01 = Enable Pull Down control
   * 10 = Enable Pull Up control
   * 11 = Reserved
   *
   */
  enum PUD { OFF = 0b00, PULL_DOWN = 0b01, PULL_UP = 0b10, RESERVED = 0b11 };

  static constexpr uint64_t gpio_base_address = GPIO_BASE;

  static constexpr uint64_t GPPUD_offset = 0x94;
  static constexpr uint64_t GPPUDCLK0_offset = 0x98;
  static constexpr uint64_t GPPUDCLK1_offset = 0x9c;

  /**
   * @brief Sets the Pull-up/down Register Value for all GPIO pins
   *
   * @details "The GPIO Pull-up/down Register controls the actuation of the
internal pull-up/down control line to ALL the GPIO pins. This register must be
used in conjunction with the 2 GPPUDCLKn registers."
   *
   * @param status  USE PUD Enum! Used to set the pull-up/down register
   *
   */
  static void set_pull_register(PUD status) {
    volatile uint32_t* GPPUD_register =
        (volatile uint32_t*)(gpio_base_address + GPPUD_offset);
    *GPPUD_register = status;
  }

  /**
   * @brief Applies the pull-up/down configuration to specific GPIO pins
   *
   * @details "The GPIO Pull-up/down Clock Registers control the actuation of
   * internal pull-downs on the respective GPIO pins. These registers must be
   * used in conjunction with the GPPUD register to effect GPIO Pull-up/down
   * changes."
   *
   * @param clock_num     0 or 1 for GPPUDCLK0 (0-31), 1 for GPPUDCLK1(32-53)
   * @param device_mask   Bitmask of which GPIO pins to apply the pull-up/down
   * setting to
   */
  static void set_clock(uint8_t clock_num, uint32_t device_mask) {
    if (clock_num == 0) {
      volatile uint32_t* GPPUDCLK0_register =
          (volatile uint32_t*)(gpio_base_address + GPPUDCLK0_offset);
      *GPPUDCLK0_register = device_mask;
    } else if (clock_num == 1) {
      volatile uint32_t* GPPUDCLK1_register =
          (volatile uint32_t*)(gpio_base_address + GPPUDCLK1_offset);
      *GPPUDCLK1_register = device_mask;
    }
  }
};