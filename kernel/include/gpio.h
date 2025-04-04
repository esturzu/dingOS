#include "definitions.h"
#include "printf.h"
#include "stdint.h"

/**
 * @brief Interface for GPIO
 *
 * Citations:
 * + https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 *
 */
class GPIO {
 public:
  /**
   * @brief PUD - GPIO Pin Pull-up/down
   * @details
   * 00 = Off – disable pull-up/down
   * 01 = Enable Pull Down control
   * 10 = Enable Pull Up control
   * 11 = Reserved
   */
  enum PUD { OFF = 0b00, PULL_DOWN = 0b01, PULL_UP = 0b10, RESERVED = 0b11 };

  static constexpr uint64_t gpio_base_address = GPIO_BASE;

  static constexpr uint64_t GPFSEL4 = gpio_base_address + 0x10;
  static constexpr uint64_t GPFSEL5 = gpio_base_address + 0x14;
  static constexpr uint64_t GPHEN0 = gpio_base_address + 0x64;
  static constexpr uint64_t GPHEN1 = gpio_base_address + 0x68;
  static constexpr uint64_t GPLEN0 = gpio_base_address + 0x70;
  static constexpr uint64_t GPLEN1 = gpio_base_address + 0x74;
  static constexpr uint64_t GPPUD = gpio_base_address + 0x94;
  static constexpr uint64_t GPPUDCLK0 = gpio_base_address + 0x98;
  static constexpr uint64_t GPPUDCLK1 = gpio_base_address + 0x9c;
  /**
   * @brief Sets the Pull-up/down Register Value for all GPIO pins
   *
   * @details "The GPIO Pull-up/down Register controls the actuation of the
   * internal pull-up/down control line to ALL the GPIO pins. This register must
   * be used in conjunction with the 2 GPPUDCLKn registers."
   *
   * @param status  USE PUD Enum! Used to set the pull-up/down register
   *
   */
  static void set_pull_register(PUD status);

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
  static void set_clock(uint8_t clock_num, uint32_t device_mask);

  /**
   * @brief Applies `and` mask to a location
   *
   * @param location    Memory location to apply the mask to
   * @param mask        Mask to apply to the location
   */
  static void maskAnd(uint64_t location, uint32_t mask);

  /**
   * @brief Applies `or` mask to a location
   *
   * @param location   Memory location to apply the mask to
   * @param mask       Mask to apply to the location
   */
  static void maskOr(uint64_t location, uint32_t mask);

  /**
   * @brief Applies a mask to a location to set the value to 0
   *
   * @param location  Memory location to apply the mask to
   */
  static void maskZero(uint64_t location);

  /**
   * @brief Set the Pull object
   *
   * @param clockNum    0 or 1 for GPPUDCLK0 (0-31), 1 for GPPUDCLK1(32-53)
   * @param pullMask    Bitmask of which GPIO pins to apply the pull-up/down
   * setting to
   * @param pud         PUD Enum! Used to set the pull-up/down register
   */
  static void setPull(uint8_t clockNum, uint32_t pullMask, PUD pud);
};