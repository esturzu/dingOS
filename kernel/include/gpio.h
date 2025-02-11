// Citation
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "stdint.h"
#include "definitions.h"

class GPIO
{

public:

  static constexpr uint64_t gpio_base_address = GPIO_BASE;

  static constexpr uint64_t GPPUD_offset = 0x94;
  static constexpr uint64_t GPPUDCLK0_offset = 0x98;
  static constexpr uint64_t GPPUDCLK1_offset = 0x9c;

 /**
   * Sets the Pull-up/down Register Value
   *
   * @param status  Status Codes: Off (00), Enable Pull Down (01), Enable Pull Up (10)
   */
  static void set_pull_register (uint8_t status)
  {
    volatile uint32_t* GPPUD_register = (volatile uint32_t*) (gpio_base_address + GPPUD_offset);
    *GPPUD_register = status;
  }

  /**
   * Sets the Pull-up/down Register Value
   *
   * @param status  Status Codes: Off (00), Enable Pull Down (01), Enable Pull Up (10)
   */
  static void set_clock (uint8_t clock_num, uint32_t device_mask)
  {
    if (clock_num == 0)
    {
      volatile uint32_t* GPPUDCLK0_register = (volatile uint32_t*) (gpio_base_address + GPPUDCLK0_offset);
      *GPPUDCLK0_register = device_mask;
    }
    else if (clock_num == 1)
    {
      volatile uint32_t* GPPUDCLK1_register = (volatile uint32_t*) (gpio_base_address + GPPUDCLK1_offset);
      *GPPUDCLK1_register = device_mask;
    }
  }
};