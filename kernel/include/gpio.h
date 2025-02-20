// Citation
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "definitions.h"
#include "stdint.h"
#include "printf.h"

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

  static constexpr uint32_t gpio_base_address = GPIO_BASE;

  static constexpr uint32_t GPFSEL4 = gpio_base_address + 0x10;
  static constexpr uint32_t GPFSEL5 = gpio_base_address + 0x14;
  static constexpr uint32_t GPHEN0 = gpio_base_address + 0x64;
  static constexpr uint32_t GPHEN1 = gpio_base_address + 0x68;
  static constexpr uint32_t GPLEN0 = gpio_base_address + 0x70;
  static constexpr uint32_t GPLEN1 = gpio_base_address + 0x74;
  static constexpr uint32_t GPPUD = gpio_base_address + 0x94;
  static constexpr uint32_t GPPUDCLK0 = gpio_base_address + 0x98;
  static constexpr uint32_t GPPUDCLK1 = gpio_base_address + 0x9c;
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
    volatile uint32_t* GPPUD_register = (volatile uint32_t*)(GPPUD);
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
      volatile uint32_t* GPPUDCLK0_register = (volatile uint32_t*)(GPPUDCLK0);
      *GPPUDCLK0_register = device_mask;
    } else if (clock_num == 1) {
      volatile uint32_t* GPPUDCLK1_register = (volatile uint32_t*)(GPPUDCLK1);
      *GPPUDCLK1_register = device_mask;
    }
  }

  enum MaskType { AND, OR, ZERO };

  /**
   * @brief Copies location value then performs mask to value based on MaskType
   * and puts it back into location
   *
   * @param location  location of value to perform mask on cast to (volatile
   * uint32_t*)
   * @param mask      mask to and apply
   */
  static void applyMask(uint32_t location, uint32_t mask, MaskType maskType) {
    volatile uint32_t* locationPTR = (volatile uint32_t*)location;
    uint32_t temp = *locationPTR;
    switch (maskType) {
      case MaskType::AND:
        temp &= mask;
        break;
      case MaskType::OR:
        temp |= mask;
        break;
      case MaskType::ZERO:
        temp = 0;
        break;
    }
    *locationPTR = temp;
  }

  static void setPull(uint8_t clockNum, uint32_t pullMask, PUD pud) {
    set_pull_register(pud);
    // // wait(150); // wait 150 cycles becuase we are supposed
    set_clock(clockNum, pullMask);

    // reset the registers back to normal
    set_pull_register(PUD::OFF);
    set_clock(1, 0);
  }

  static void eMMCinit() {
    // *** setting up the SD card detect pin (pin47) by masking out its 3
    // alternate clear out any alternate function selection for pin47
    applyMask(GPFSEL4, ~(7 << (7 * 3)), MaskType::AND);

    // set pin47 to be pull up
    setPull(1, 1 << 15, PUD::PULL_UP);

    // now we need to set that pin to also be high detect
    applyMask(GPHEN1, 1 << 15, MaskType::OR);

    // TODO: I truly have no idea why this is initatilizing any of these pins
    // because according to documentation these pins should not be used. it
    // might just be an error with the BCM2837 documentation but i cant find
    // anything about alternate fucntions in more detail for now i shall assume
    // that
    // https://github.com/bztsrc/raspi3-tutorial/blob/master/0B_readsector/sd.c#L91
    // knows what they are doing

    // *** set up pins 48 and 49 for eMMC? supposedlyf for "GPIO_CLK, GPIO_CMD"
    // set alt function 3 for 48 and 49
    applyMask(GPFSEL4, (3 << (7 * 3)) | (3 << (8 * 3)), MaskType::OR);
    // set pull up for 48 and 49
    setPull(1, (1 << 16) | (1 << 17), PUD::PULL_UP);

    // *** set up pins 50-53 for eMMC? supposedly for "GPIO_DAT0, GPIO_DAT1,
    // GPIO_DAT2, GPIO_DAT3" set alt function 3 for 50-53

    applyMask(GPFSEL5,
              (3 << (0 * 3)) | (3 << (1 * 3)) | (3 << (2 * 3)) | (3 << (3 * 3)),
              MaskType::OR);
    // set pull up for 50-53
    setPull(1, (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21), PUD::PULL_UP);

  }
};