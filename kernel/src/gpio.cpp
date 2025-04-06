#include "gpio.h"

void GPIO::set_pull_register(PUD status) {
  volatile uint32_t* GPPUD_register = (volatile uint32_t*)(GPPUD);
  *GPPUD_register = status;
}

void GPIO::set_clock(uint8_t clock_num, uint32_t device_mask) {
  volatile uint32_t* GPPUDCLK_register =
      (volatile uint32_t*)(GPPUDCLK0) + clock_num;
  *GPPUDCLK_register = device_mask;
}

void GPIO::maskAnd(uint64_t location, uint32_t mask) {
  volatile uint32_t* locationPTR = (volatile uint32_t*)location;
  debug_printf("location: 0x%lx\n", locationPTR);
  uint32_t temp = *locationPTR;
  *locationPTR &= mask;
}

void GPIO::maskOr(uint64_t location, uint32_t mask) {
  volatile uint32_t* locationPTR = (volatile uint32_t*)location;
  uint32_t temp = *locationPTR;
  *locationPTR |= mask;
}

void GPIO::maskZero(uint64_t location) {
  volatile uint32_t* locationPTR = (volatile uint32_t*)location;
  *locationPTR = 0;
}

void GPIO::setPull(uint8_t clockNum, uint32_t pullMask, PUD pud) {
  set_pull_register(pud);
  // wait(150); // wait 150 cycles becuase we are supposed
  set_clock(clockNum, pullMask);

  // reset the registers back to normal
  set_pull_register(PUD::OFF);
  set_clock(1, 0);
}