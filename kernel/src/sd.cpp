
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
volatile uint32_t* SD::cardConfigRegister1 = 0;
volatile uint32_t* SD::cardConfigRegister2 = 0;
uint32_t SD::relativeCardAddress = 0;

/**
 * @brief Waits for an interrupt to be set for the given mask or a timeout/error
 *
 * @param mask              mask of the interrupt to wait for
 * @param timeout           number of times to check before timing out
 * @return SD::RESPONSE     result of the wait
 */
SD::RESPONSE SD::waitInterrupt(uint32_t mask, uint32_t timeout) {
  // wait for a masked interrupt to be set or timeout
  mask |= INT_ERROR_MASK;  // always want to check for errors
  while (!(*EMMC_INTERRUPT & mask) && timeout--) {
    // wait
    debug_printf("Waiting for EMMC interrupt\n");
  }

  // check if we timed out
  uint32_t interruptValue = *EMMC_INTERRUPT;
  if (timeout <= 0 || (interruptValue & INT_TIMEOUT_MASK)) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    debug_printf("Timeout waiting for interrupt\n");
    return TIMEOUT;
  }

  // check if there was an error
  if (interruptValue & INT_ERROR_MASK) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    debug_printf("Error waiting for interrupt\n");
    return ERROR;
  }

  // success!
  *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
  return SUCCESS;
}

/**
 * @brief Waits for the status register to have 0s in the bits specified by mask
 *
 * @param mask              what bits to wait for 0 from status register
 * @param timeout           number of times to check before timing out
 * @return SD::RESPONSE     result of the wait
 */
SD::RESPONSE SD::waitStatus(uint32_t mask, uint32_t timeout) {
  while ((*EMMC_STATUS & mask) && !(*EMMC_INTERRUPT & INT_ERROR_MASK) &&
         timeout--) {
    // TODO: add a delay
    debug_printf("Waiting for SD status to be ready\n");
  }

  if (timeout <= 0) {
    debug_printf("Timeout waiting for SD status to be ready\n");
    return TIMEOUT;
  }

  if (*EMMC_INTERRUPT & INT_ERROR_MASK) {
    debug_printf("Error waiting for SD status to be ready\n");
    return ERROR;
  }
  return SUCCESS;
}

/**
 * @brief Sends a command to the SD card
 *
 * @param cmd               command to send
 * @param arg               arguments to send with the command
 * @return SD::RESPONSE     result of the command
 */
SD::RESPONSE SD::sendCommand(CMDS cmd, uint32_t arg) {
  RESPONSE myResponse = SUCCESS;
  // check if the command is app specific
  switch (cmd) {
    case CMDS::SET_BUS_WIDTH:
    case CMDS::SEND_OP_COND:
    case CMDS::SEND_SCR:
      // if we are in relative address mode we need to add flag for that and
      // include it in args
      CMDS appCMD = relativeCardAddress ? APP_CMD_RCA : APP_CMD;
      // if it is app specific then we need to send the app command first

      myResponse = sendCommand(appCMD, relativeCardAddress);

      if (relativeCardAddress && myResponse < SUCCESS) {
        printf("ERROR: Failed to send app command\n");
        return myResponse;
      }
  }

  // now check if any commands are already running
  if ((myResponse = waitStatus(0b1)) <
      SUCCESS) {  // TODO: i wonder if there should be a
                  // lock or something for this?
    printf("ERROR: Failed while waiting for EMMC to be free\n");
    return myResponse;
  }

  // now we actually setup and send the command
  *EMMC_INTERRUPT = *EMMC_INTERRUPT;  // clear interrupts
  *EMMC_ARG1 = arg;
  *EMMC_CMDTM = (uint32_t)cmd;

  switch (cmd) {
    case CMDS::SEND_OP_COND:
      // should be waiting 1000 ms here
      break;
    case CMDS::SEND_IF_COND:
    case CMDS::APP_CMD:
      // should be waiting 100 ms here
      break;
  }

  // wait for interrupt indicating command is done

  if ((myResponse = waitInterrupt(INT_CMD_DONE)) < SUCCESS) {
    printf("ERROR: failed to send EMMC command\n");
    return myResponse;
  }
  uint32_t cmdResponse = *EMMC_RESP0;

  debug_printf("||CMD: %x | ARG: %x\n", cmd, arg);
  debug_printf("||CMD_RESP: %x\n", cmdResponse);
  switch (cmd) {
    case CMDS::GO_IDLE:
    case CMDS::APP_CMD:
      return SUCCESS;
    case CMDS::APP_CMD_RCA:
      return SUCCESS;  // for some reason we want a parameter error????
  }
  //   if (code == CMD_GO_IDLE || code == CMD_APP_CMD)
  //     return 0;
  //   else if (code == (CMD_APP_CMD | CMD_RSPNS_48))
  //     return r & SR_APP_CMD;
  //   else if (code == CMD_SEND_OP_COND)
  //     return r;
  //   else if (code == CMD_SEND_IF_COND)
  //     return r == arg ? SD_OK : SD_ERROR;
  //   else if (code == CMD_ALL_SEND_CID) {
  //     r |= *EMMC_RESP3;
  //     r |= *EMMC_RESP2;
  //     r |= *EMMC_RESP1;
  //     return r;
  //   } else if (code == CMD_SEND_REL_ADDR) {
  //     sd_err = (((r & 0x1fff)) | ((r & 0x2000) << 6) | ((r & 0x4000) << 8) |
  //               ((r & 0x8000) << 8)) &
  //              CMD_ERRORS_MASK;
  //     return r & CMD_RCA_MASK;
  //   }
  //   return r & CMD_ERRORS_MASK;

  return myResponse;
}

SD::RESPONSE SD::setClock(uint32_t freq) {
  // first wait for command and data lines to be free "Command line still used
  // by previous command" bit 0 and "Data lines still used by previous data
  // transfer" bit 1 to be 0
  uint8_t timeout = 0xFF;

  while ((*EMMC_STATUS & 0b11) && timeout--) {
    // wait
    debug_printf("Waiting for command and data lines to be free\n");
  }
  if (timeout <= 0) {
    debug_printf("Timeout waiting for command and data lines to be free\n");
    return TIMEOUT;
  }
  debug_printf("Done waiting for command and data to be free\n");

  // disable "SD clock enable"
  *EMMC_CONTROL0 &= ~((uint32_t)0b100);

  // finding how much we have to slow down the clock by
  // finding the log2 of the frequency
  uint32_t shift, x = 41666666 / freq -
                      1;  // 41666666 is the base frequency of system clock
  shift = (x == 0) ? 0 : (31 - __builtin_clz(x));
  if (shift > 7) shift = 7;

  //  if(hv>HOST_SPEC_V2) d=c; else d=(1<<s); i am guessing this is for if
  //  the host is above 2 but i think we dont need to worry about?
  uint32_t divisor = (1 << shift);
  if (divisor <= 2) {
    divisor = 2;
    shift = 0;
  }
  debug_printf("divisor: %u | shift: %u\n", divisor, shift);

  // if(hv>HOST_SPEC_V2) h=(d&0x300)>>2; // TODO: doesnt matter assusming
  // version doesnt go above 2 (once this does matter we nee to also change the
  // next true line of code)

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
    debug_printf("Waiting for clock to be stable\n");
  }

  if (timeout <= 0) {
    return TIMEOUT;
  }
  debug_printf("Done waiting for clock to be stable\n");

  return SUCCESS;
}

SD::RESPONSE SD::init() {
  uint32_t timeout = 1000000;
  RESPONSE response = SUCCESS;
  GPIO::eMMCinit();

  // *** this part starts to make a little more sense. we are now doing very
  // standard setup for EMMC

  // first try to get the sd version (i think this just reads as 0 tho)
  SLOTISR_VER = (*EMMC_SLOTISR_VER);
  SLOT_STATUS = SLOTISR_VER & 0xFF;        // bits 0-7
  SDVERSION = (SLOTISR_VER >> 16) & 0xFF;  // bits 16-23
  debug_printf("SD slot status: %d\n", SLOT_STATUS);
  debug_printf("SD version: %d\n", SDVERSION);

  // resetting the sd card
  *EMMC_CONTROL0 = 0x00000000;
  *EMMC_CONTROL1 |= 0x01000000;  // "Reset the complete host circuit"

  timeout = 1000;
  while ((*EMMC_CONTROL1 & 0x01000000) && timeout--) {
    debug_printf("Waiting for EMMC host circuit reset...\n");
  };
  if (timeout <= 0) {
    debug_printf("Timeout waiting for EMMC host circuit reset\n");
    return TIMEOUT;
  }
  debug_printf("Done with EMMC host circuit reset\n");

  // "Clock enable for internal EMMC clocks for powersaving" at bit 0 and
  // "Data timeout unit exponent" to 7 at bit 16
  *EMMC_CONTROL1 = 1 | (7 << 16);

  // setup starting frequency (required to be slow at first for
  // initialization)

  if ((response = setClock(400000)) != SUCCESS) {
    debug_printf("Failed to set clock frequency\n");
    return response;
  }

  // enable all interrupts
  *EMMC_IPRT_EN = 0xFFFFFFFF;
  *EMMC_IRPT_MASK = 0xFFFFFFFF;

  // initalize some variables
  cardConfigRegister1 = 0;
  cardConfigRegister2 = 0;
  relativeCardAddress = 0;

  sendCommand(CMDS::GO_IDLE, 0);

  return SUCCESS;
}