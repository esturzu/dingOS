
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
volatile uint64_t* SD::cardConfigRegister1 = 0;
volatile uint64_t* SD::cardConfigRegister2 = 0;
uint32_t SD::relativeCardAddress = 0;
uint32_t SD::errInfo = 0;

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
    // wait(100);
    debug_printf("Waiting for EMMC interrupt\n");
  }

  // check if we timed out
  uint32_t interruptValue = *EMMC_INTERRUPT;
  if (timeout <= 0 || (interruptValue & INT_TIMEOUT_MASK)) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    error_printf("Error: Timeout waiting for interrupt\n");
    return TIMEOUT;
  }

  // check if there was an error
  if (interruptValue & INT_ERROR_MASK) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    error_printf("Error: waiting for interrupt\n");
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
    // wait(100);
    debug_printf("Waiting for SD status to be ready\n");
  }

  if (timeout <= 0) {
    error_printf("Error: Timeout waiting for SD status to be ready\n");
    return TIMEOUT;
  }

  if (*EMMC_INTERRUPT & INT_ERROR_MASK) {
    error_printf("Error: waiting for SD status to be ready\n");
    return ERROR;
  }
  return SUCCESS;
}

/**
 * @brief Sends a command to the SD card
 * Note! RELATIVE_CARD_ADDRESS doesnt not return normaly. the rca is returned in
 * the response and the error info is returned in the errInfo variable with the
 * bits in the correct place
 *
 * @param cmd               command to send
 * @param arg               arguments to send with the command
 * @return SD::RESPONSE     result of the command
 */
uint32_t SD::sendCommand(CMDS cmd, uint32_t arg) {
  uint32_t myResponse = SUCCESS;
  errInfo = 0;
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
        error_printf("ERROR: Failed to send app command\n");
        errInfo = ERROR;
        return myResponse;
      }
  }

  // now check if any commands are already running
  if ((myResponse = waitStatus(0b1)) <
      SUCCESS) {  // TODO: i wonder if there should be a
                  // lock or something for this?
    error_printf("ERROR: Failed while waiting for EMMC to be free\n");
    errInfo = TIMEOUT;
    return myResponse;
  }

  // now we actually setup and send the command
  *EMMC_INTERRUPT = *EMMC_INTERRUPT;  // clear interrupts
  *EMMC_ARG1 = arg;
  *EMMC_CMDTM = (uint32_t)cmd;

  switch (cmd) {
    case CMDS::SEND_OP_COND:
      // wait(1000);
      break;
    case CMDS::SEND_IF_COND:
    case CMDS::APP_CMD:
      // wait(100);
      break;
  }

  // wait for interrupt indicating command is done
  if ((myResponse = waitInterrupt(0x1)) != SUCCESS) {
    error_printf("ERROR: failed to send EMMC command (0x%08x)\n", cmd);
    errInfo = myResponse;
    return myResponse;
  }

  uint32_t cmdResponse =
      *EMMC_RESP0;  // half a real response and half the cards status info from
                    // before and after the command

  debug_printf("||CMD: %x | ARG: %x\n", cmd, arg);
  debug_printf("||CMD_RESP: %b\n", cmdResponse);
  switch (cmd) {
    case CMDS::GO_IDLE:
    case CMDS::APP_CMD:
      return SUCCESS;
    case CMDS::APP_CMD_RCA:
      return cmdResponse & SD_APP_CMD_ENABLED
                 ? SUCCESS
                 : FAIL;  // check to make sure that the stuatus indicates that
                          // the card is ready for application specific commands
    case CMDS::SEND_OP_COND:
      return cmdResponse;
    case CMDS::SEND_IF_COND:
      return cmdResponse == arg ? SUCCESS : ERROR;
    case CMDS::ALL_SEND_CID:
      return *EMMC_RESP3 | *EMMC_RESP2 | *EMMC_RESP1 |
             cmdResponse;  // TODO: returning the whole response??
    case CMDS::SEND_REL_ADDR:
      // response has lower half error info and uppper half RCA
      errInfo = ((cmdResponse & 0x1FFF) | ((cmdResponse & (1 << 13)) << 6) |
                 ((cmdResponse & (3 << 14)) << 8)) &
                ERRORS_MASK;  // lower 13 bits are in normal place and upper
                              // 3 bits have to be moved to right place 13
                              // -> 19, 14 ->22, 15 ->23

      return (cmdResponse & 0xFFFF0000);
  }
  return cmdResponse & ERRORS_MASK;
}

SD::RESPONSE SD::setClock(uint32_t freq) {
  // first wait for command and data lines to be free "Command line still used
  // by previous command" bit 0 and "Data lines still used by previous data
  // transfer" bit 1 to be 0
  uint8_t timeout = 0xFF;

  while ((*EMMC_STATUS & 0b11) && timeout--) {
    // wait(100);
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
    // wait(100);
    debug_printf("Waiting for clock to be stable\n");
  }

  if (timeout <= 0) {
    return TIMEOUT;
  }
  debug_printf("Done waiting for clock to be stable\n");

  return SUCCESS;
}

uint32_t SD::init() {
  uint32_t timeout = 1000000;
  uint32_t response = SUCCESS;
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
    error_printf("Error: Waiting for EMMC host circuit reset...\n");
  };
  if (timeout <= 0) {
    error_printf("Error: Timeout waiting for EMMC host circuit reset\n");
    return TIMEOUT;
  }
  debug_printf("Done with EMMC host circuit reset\n");

  // "Clock enable for internal EMMC clocks for powersaving" at bit 0 and
  // "Data timeout unit exponent" to 7 at bit 16
  *EMMC_CONTROL1 = 1 | (7 << 16);

  // setup starting frequency (required to be slow at first for
  // initialization)

  if ((response = setClock(400000)) != SUCCESS) {
    error_printf("Error to set clock frequency\n");
    return response;
  }

  // enable all interrupts
  *EMMC_IPRT_EN = 0xFFFFFFFF;
  *EMMC_IRPT_MASK = 0xFFFFFFFF;

  // initalize some variables
  cardConfigRegister1 = 0;
  cardConfigRegister2 = 0;
  relativeCardAddress = 0;
  errInfo = 0;

  sendCommand(CMDS::GO_IDLE, 0);
  if (errInfo) {
    return ERROR;
  }

  sendCommand(SEND_IF_COND, 0x000001AA);  // AA is the check pattern and 1 is
                                          // the voltage range (2.7-3.6V)
  if (errInfo) {
    return ERROR;
  }

  // trying to start the initialization process
  const uint32_t isCompleteMask = 1 << 31;
  timeout = 6;
  while (timeout--) {
    // wait(400);
    response = sendCommand(
        SEND_OP_COND,
        OP_COND_FULL);  // sending the voltage window for initalization

    if (response & isCompleteMask) {  // check if it is still busy trying to
                                      // initialize the card
      break;
    }
    debug_printf(
        "During initialization, card is still busy\n With this response: %x\n",
        response);

    if (errInfo != TIMEOUT && errInfo != SUCCESS) {
      error_printf("Error: during sd initialization\n");
      return errInfo;
    }
  }

  // check if we timed out
  if (timeout <= 0) {
    error_printf("Error: Timeout during sd initialization\n");
    return TIMEOUT;
  }

  // check if choosing the voltage window was successful
  if (!(response & VOLTAGE_WINDOW)) {
    error_printf("Error: Failed to choose voltage window\n");
    return ERROR;
  }

  // check for ccs bit
  uint32_t ccs = response & CCS_MASK;

  bool isNotSDHC =
      response & OP_COND_NOT_SDSC;  // check if SCDC and SDXC are both supported

  // required for some reason
  sendCommand(ALL_SEND_CID, 0);

  // try to get the relative card address
  relativeCardAddress = sendCommand(SEND_REL_ADDR, 0);
  if (errInfo) {
    error_printf("Error: Failed to get relative card address\n");
    error_printf("Error response: 0x%x\n", relativeCardAddress);
    error_printf("Error info: 0x%x\n", errInfo);
    return ERROR;
  }

  // setting the clock to the real clock speed
  if ((response = setClock(25000000)) != SUCCESS) {
    error_printf("Error: Failed to set clock speed\n");
    return response;
  }

  // select the card
  sendCommand(CARD_SELECT, relativeCardAddress);
  if (errInfo) {
    error_printf("Error: Failed to select card\n");
    return ERROR;
  }

  // wait for no data lines to be in use
  if (response = waitStatus(0x2)) return response;

  // set the block size to ___ bytes
  *EMMC_BLKSIZECNT =
      1 << 10;  // TODO: this is weird, idk how big the block size should be
  // try to get SD to send SCR
  sendCommand(SEND_SCR, 0);
  if (errInfo) {
    error_printf("Error: Failed to send SCR command\n");
    return ERROR;
  }

  // wait for interrupt indicating read is ready
  if ((response = waitInterrupt(0x20)) != SUCCESS) {
    error_printf("Error: failed to read EMMC\n");
    return response;
  }

  // keep trying to get both of the SCR registers
  timeout = 1000000;
  response = 0;
  volatile uint64_t** currentRegister = &cardConfigRegister1;
  while (timeout--) {
    // read the data
    if (*EMMC_STATUS & SD_READ_AVAILABLE) {
      **currentRegister = *EMMC_DATA;
      if (currentRegister == &cardConfigRegister2) {
        // we have set both registers
        break;
      }
      currentRegister = &cardConfigRegister2;
    } else {
      // wait(500);
      debug_printf("Waiting for cardConfigRegisters to be available\n");
    }
  }
  if (timeout <= 0) {
    error_printf(
        "Error: Timeout waiting for cardConfigRegisters to be available\n");
    return TIMEOUT;
  }

  // check for bus width 4 availability
  if (*cardConfigRegister1 & 1 << 10) {
    sendCommand(SET_BUS_WIDTH,
                relativeCardAddress | 0x2);  // try to set the bus width to 4
                                             // using the relative card address
  }
  if (errInfo) {
    error_printf("Error: Failed to set bus width\n");
    return ERROR;
  }
  // update contol 0 to enable 4 data lines
  *EMMC_CONTROL0 |= 0x2;

  // check if it supports SET_BLKCNT command
  if (*cardConfigRegister1 & 1 << 25) {
    // enable multiple block read
    printf("SD supports SET_BLKCNT command\n");
  }
  if (ccs) {
    printf("SD is SDHC and SDXC compatiable\n");
  }

  // make sure config is set to yes for SDHC and SDXC
  *cardConfigRegister1 &= ~ccs;
  *cardConfigRegister1 |= ccs;

  return SUCCESS;
}

uint32_t SD::read(uint32_t block, uint32_t count, uint8_t* buffer) {
  uint32_t response = SUCCESS;
  // first wait for data lines to be free
  if ((response = waitStatus(0b10)) < SUCCESS) {
    error_printf(
        "Error: Failed while waiting for EMMC data lines to be free\n");
    return response;
  }

  uint8_t* bufferPtr = buffer;

  // check if we are supporting SDHC and SDXC
  if (*cardConfigRegister1 & CCS_MASK) {
    // check if we are supporting set block count
  }

  return SUCCESS;
}
uint32_t SD::write(uint32_t block, uint32_t count, uint8_t* buffer) {
  return SUCCESS;
}