
#include "sd.h"

volatile uint32_t* const SD::EMMC_BASE =
    (volatile uint32_t*)0x3F300000;  // 0x3F300000 + 0x00
volatile uint32_t* const SD::EMMC_ARG2 =
    (volatile uint32_t*)0x3F300000;  // 0x3F300000 + 0x00
volatile uint32_t* const SD::EMMC_BLKSIZECNT =
    (volatile uint32_t*)0x3F300004;  // 0x3F300000 + 0x04
volatile uint32_t* const SD::EMMC_ARG1 =
    (volatile uint32_t*)0x3F300008;  // 0x3F300000 + 0x08
volatile uint32_t* const SD::EMMC_CMDTM =
    (volatile uint32_t*)0x3F30000C;  // 0x3F300000 + 0x0C
volatile uint32_t* const SD::EMMC_RESP0 =
    (volatile uint32_t*)0x3F300010;  // 0x3F300000 + 0x10
volatile uint32_t* const SD::EMMC_RESP1 =
    (volatile uint32_t*)0x3F300014;  // 0x3F300000 + 0x14
volatile uint32_t* const SD::EMMC_RESP2 =
    (volatile uint32_t*)0x3F300018;  // 0x3F300000 + 0x18
volatile uint32_t* const SD::EMMC_RESP3 =
    (volatile uint32_t*)0x3F30001C;  // 0x3F300000 + 0x1C
volatile uint32_t* const SD::EMMC_DATA =
    (volatile uint32_t*)0x3F300020;  // 0x3F300000 + 0x20
volatile uint32_t* const SD::EMMC_STATUS =
    (volatile uint32_t*)0x3F300024;  // 0x3F300000 + 0x24
volatile uint32_t* const SD::EMMC_CONTROL0 =
    (volatile uint32_t*)0x3F300028;  // 0x3F300000 + 0x28
volatile uint32_t* const SD::EMMC_CONTROL1 =
    (volatile uint32_t*)0x3F30002C;  // 0x3F300000 + 0x2C
volatile uint32_t* const SD::EMMC_INTERRUPT =
    (volatile uint32_t*)0x3F300030;  // 0x3F300000 + 0x30
volatile uint32_t* const SD::EMMC_IRPT_MASK =
    (volatile uint32_t*)0x3F300034;  // 0x3F300000 + 0x34
volatile uint32_t* const SD::EMMC_IPRT_EN =
    (volatile uint32_t*)0x3F300038;  // 0x3F300000 + 0x38
volatile uint32_t* const SD::EMMC_CONTROL2 =
    (volatile uint32_t*)0x3F30003C;  // 0x3F300000 + 0x3c
volatile uint32_t* const SD::EMMC_FORCE_IRPT =
    (volatile uint32_t*)0x3F300050;  // 0x3F300000 + 0x50
volatile uint32_t* const SD::EMMC_BOOT_TIMEOUT =
    (volatile uint32_t*)0x3F300070;  // 0x3F300000 + 0x70
volatile uint32_t* const SD::EMMC_DBG_SEL =
    (volatile uint32_t*)0x3F300074;  // 0x3F300000 + 0x74
volatile uint32_t* const SD::EMMC_EXRDFIFIO_CFG =
    (volatile uint32_t*)0x3F300080;  // 0x3F300000 + 0x80
volatile uint32_t* const SD::EMMC_EXRDFIFO_EN =
    (volatile uint32_t*)0x3F300084;  // 0x3F300000 + 0x84
volatile uint32_t* const SD::EMMC_TUNE_STEP =
    (volatile uint32_t*)0x3F300088;  // 0x3F300000 + 0x88
volatile uint32_t* const SD::EMMC_TUNE_STEP_STD =
    (volatile uint32_t*)0x3F30008C;  // 0x3F300000 + 0x8C
volatile uint32_t* const SD::EMMC_TUNE_STEP_DDR =
    (volatile uint32_t*)0x3F300090;  // 0x3F300000 + 0x90
volatile uint32_t* const SD::EMMC_INT_SPI =
    (volatile uint32_t*)0x3F3000F0;  // 0x3F300000 + 0xF0
volatile uint32_t* const SD::EMMC_SLOTISR_VER =
    (volatile uint32_t*)0x3F3000FC;  // 0x3F300000 + 0xFC

uint32_t SD::SLOTISR_VER = 0;
uint32_t SD::SLOT_STATUS = 0;
uint32_t SD::SDVERSION = 0;

SD::RESPONSE SD::setClock(uint32_t freq) {
  // first wait for command and data lines to be free "Command line still used
  // by previous command" bit 0 and "Data lines still used by previous data
  // transfer" bit 1 to be 0
  uint8_t timeout = 0xFF;

  while ((*EMMC_STATUS & 0b11) && timeout--) {
// wait
#ifndef DEBUG_ENABLED
    Debug::printf("Waiting for command and data to be free\n");
#endif
  }
  if (timeout <= 0) {
    return TIMEOUT;
  }
  Debug::printf("Done waiting for command and data to be free\n");

  // disable "SD clock enable"
  *EMMC_CONTROL0 &= ~((uint32_t)0b100);

  // finding how much we have to slow down the clock by
  // finding the log2 of the frequency
  uint32_t shift, x = 41666666 / freq -
                      1;  // 41666666 is the base frequency of system clock
  shift = (x == 0) ? 0 : (31 - __builtin_clz(x));
  if (shift > 7) shift = 7;

  //  if(sd_hv>HOST_SPEC_V2) d=c; else d=(1<<s); i am guessing this is for if
  //  the host is above 2 but i think we dont need to worry about?
  uint32_t divisor = (1 << shift);
  if (divisor <= 2) {
    divisor = 2;
    shift = 0;
  }
#ifdef DEBUG_ENABLED
  Debug::printf("divisor: %u | shift: %u\n", divisor, shift);
#endif

  // if(sd_hv>HOST_SPEC_V2) h=(d&0x300)>>2; // doesnt matter asusming version
  // doesnt go above 2 (once this does matter we nee to also change the next
  // true line of code)

  // moving the divisor into the correct place
  divisor = (divisor & 0xFF) << 8;

  // putting in our own divisor into "SD clock base divider LSBs" (bits 8-15)
  // and clearing "SD clock base divider MSBs" (bits 6-7)
  constexpr uint32_t mask6_15 = 0x3FF << 6;
  *EMMC_CONTROL1 = *EMMC_CONTROL1 & (~mask6_15) | divisor;

  // renable "SD clock enable"
  *EMMC_CONTROL1 |= 0b100;

  // wait for the clock to be stable
  timeout = 0xFF;
  while (!(*EMMC_STATUS & 0b10) && timeout--) {
    // wait
    Debug::printf("Waiting for clock to be stable\n");
  }

  if (timeout <= 0) {
    return TIMEOUT;
  }
#ifdef DEBUG_ENABLED
  Debug::printf("Done waiting for clock to be stable\n");
#endif

  return SUCCESS;
}

SD::RESPONSE SD::init() {














  //*********************************** make sure to check that cpu is not cacheing these addresses





























  RESPONSE response = SUCCESS;
  GPIO::eMMCinit();

  // *** this part starts to make a little more sense. we are now doing very
  // standard setup for EMMC

  // first try to get the sd version (i think this just reads as 0 tho)
  SLOTISR_VER = (*EMMC_SLOTISR_VER);
  SLOT_STATUS = SLOTISR_VER & 0xFF;        // bits 0-7
  SDVERSION = (SLOTISR_VER >> 16) & 0xFF;  // bits 16-23
  Debug::printf("SD slot status: %d\n", SLOT_STATUS);
  Debug::printf("SD version: %d\n", SDVERSION);

  // resetting the sd card
  *EMMC_CONTROL0 = 0x00000000;
  *EMMC_CONTROL1 |= 0x01000000;  // "Reset the complete host circuit"
// keep trying to reset until the reset bit is cleared
// #ifdef QEMU_ENABLED  // QEMU does not support this TODO: menntion this in
//   // person
//   Debug::printf("Unable to reset the SD card in QEMU\n");
// #else
//   while (!(*EMMC_CONTROL1 & 0x01000000)) {
//     Debug::printf("Waiting for reset...\n");
//   };
// // #endif
//   Debug::printf("SD reset complete\n");

  // "Clock enable for internal EMMC clocks for powersaving" at bit 0 and
  // "Data timeout unit exponent" to 7 at bit 16
  *EMMC_CONTROL1 = 1 | (7 << 16);

  // setup starting frequency (required to be slow at first for
  // initialization)

  if ((response = setClock(400000)) != SUCCESS) {
    Debug::printf("Failed to set clock frequency\n");
    return response;
  }

  return SUCCESS;
}