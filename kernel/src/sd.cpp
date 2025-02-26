
#include "sd.h"

// SD emmc registers
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

// Initializing static variables
uint32_t SD::SLOTISR_VER = 0;
uint32_t SD::SLOT_STATUS = 0;
uint32_t SD::SDVERSION = 0;
uint64_t SD::cardConfigRegister1 = 0;
uint64_t SD::cardConfigRegister2 = 0;
uint32_t SD::relativeCardAddress = 0;
uint32_t SD::errInfo = 0;
bool SD::ccs = 0;
bool SD::canSetBlockCount = 0;

SD::RESPONSE SD::waitInterrupt(uint32_t mask, uint32_t timeout) {
  // make sure we are will stop waiting if we get an error
  mask |= INT_ERROR_MASK;
  while (!(*EMMC_INTERRUPT & mask) && timeout--) {
    // wait(100); // TODO: swap to non busy waiting
  }

  // check if we timed out
  uint32_t interruptValue = *EMMC_INTERRUPT;
  if (timeout <= 0 || (interruptValue & INT_TIMEOUT_MASK)) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    error_printf(
        "Error: Timeout waiting for interrupt. EMMC_INTERRUPT=(0x%08x)\n",
        interruptValue);
    return TIMEOUT;
  }

  // check if there was an error
  if (interruptValue & INT_ERROR_MASK) {
    *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
    error_printf("Error: waiting for interrupt EMMC_INTERRUPT=(0x%08x)\n",
                 interruptValue);
    return ERROR;
  }

  // success!
  *EMMC_INTERRUPT = interruptValue;  // sending acknowledgement
  return SUCCESS;
}

SD::RESPONSE SD::waitStatus(uint32_t mask, uint32_t timeout) {
  // wait for the status register to have 0s in the bits specified by mask and
  // no errors
  while ((*EMMC_STATUS & mask) && !(*EMMC_INTERRUPT & INT_ERROR_MASK) &&
         timeout--) {
    // wait(100); // TODO: swap to non busy waiting
  }

  uint32_t interruptValue = *EMMC_INTERRUPT;
  // check if we timed out
  if (timeout <= 0 || (interruptValue & INT_TIMEOUT_MASK)) {
    error_printf(
        "Error: Timeout waiting for SD status to be ready "
        "EMMC_INTERRUPT=(0x%08x)\n",
        interruptValue);
    return TIMEOUT;
  }

  // check if there was an error
  if (interruptValue & INT_ERROR_MASK) {
    error_printf(
        "Error: waiting for SD status to be ready EMMC_INTERRUPT=(0x%08x)\n",
        interruptValue);
    return ERROR;
  }
  return SUCCESS;
}

uint32_t SD::sendCommand(CMDS cmd, uint32_t arg) {
  debug_printf("SD::sendCommand: cmd 0x%08x || arg 0x%08x\n", cmd, arg);
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

      // check if the app specific command was successful
      if (relativeCardAddress && myResponse < SUCCESS) {
        error_printf("ERROR: Failed to send app command\n");
        errInfo = ERROR;
        return myResponse;
      }
  }

  // now check if any commands are already running
  if ((myResponse = waitStatus(CMD_BUSY)) != SUCCESS) {
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
      // wait(1000); // TODO: add a wait
      break;
    case CMDS::SEND_IF_COND:
    case CMDS::APP_CMD:
      // wait(100); // TODO: add a wait
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
  debug_printf("SD::sendCommand: sd card Response: 0x%08x\n", cmdResponse);

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
  RESPONSE r = waitStatus(0b11);
  if (r != SUCCESS) {
    error_printf(
        "ERROR: Failed waiting for command and data lines to be free\n");
    return r;
  }

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
  r = waitStatus(DATA_BUSY);
  if (r != SUCCESS) {
    error_printf("ERROR: Failed while waiting for the clock to be stable\n");
    return r;
  }

  return SUCCESS;
}

void SD::eMMCinit() {
  // *** setting up the SD card detect pin (pin47) by masking out its 3
  // alternate clear out any alternate function selection for pin47
  GPIO::maskAnd(GPIO::GPFSEL4, ~(7 << (7 * 3)));

  // set pin47 to be pull up
  GPIO::setPull(1, 1 << 15, GPIO::PUD::PULL_UP);

  // now we need to set that pin to also be high detect
  GPIO::maskOr(GPIO::GPHEN1, 1 << 15);

  // *** set up pins 48 and 49 for eMMC? supposedly for "GPIO_CLK, GPIO_CMD"
  // set alt function 3 for 48 and 49
  GPIO::maskOr(GPIO::GPFSEL4, (3 << (7 * 3)) | (3 << (8 * 3)));
  // set pull up for 48 and 49
  GPIO::setPull(1, (1 << 16) | (1 << 17), GPIO::PUD::PULL_UP);

  // *** set up pins 50-53 for eMMC? supposedly for "GPIO_DAT0, GPIO_DAT1,
  // GPIO_DAT2, GPIO_DAT3" set alt function 3 for 50-53

  GPIO::maskOr(GPIO::GPFSEL5, (3 << (0 * 3)) | (3 << (1 * 3)) | (3 << (2 * 3)) |
                                  (3 << (3 * 3)));
  // set pull up for 50-53
  GPIO::setPull(1, (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21),
                GPIO::PUD::PULL_UP);
}

uint32_t SD::init() {
  uint32_t timeout = 1000000;
  uint32_t response = SUCCESS;
  eMMCinit();

  // *** this part starts to make a little more sense. we are now doing very
  // standard setup for EMMC

  // first get info about the sd card
  SLOTISR_VER = (*EMMC_SLOTISR_VER);
  SLOT_STATUS = SLOTISR_VER & 0xFF;        // bits 0-7
  SDVERSION = (SLOTISR_VER >> 16) & 0xFF;  // bits 16-23

  // resetting the sd card
  *EMMC_CONTROL0 = 0x00000000;
  *EMMC_CONTROL1 |= 0x01000000;  // "Reset the complete host circuit"

  timeout = 1000;
  // wait for the reset to finish
  while ((*EMMC_CONTROL1 & 0x01000000) && timeout--) {
    // TODO: add a wait
  };
  if (timeout <= 0) {
    error_printf("Error: Timeout waiting for EMMC host circuit reset\n");
    return TIMEOUT;
  }
  // "Clock enable for internal EMMC clocks for powersaving" at bit 0 and
  // "Data timeout unit exponent" to 7 at bit 16
  *EMMC_CONTROL1 = 1 | (7 << 16);

  // setup starting frequency (required to be slow at first for
  // initialization)

  if ((response = setClock(400000)) != SUCCESS) {
    error_printf("ERROR:  Unable to set clock frequency to 400000\n");
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

  // next step is to send the GO_IDLE command
  sendCommand(GO_IDLE, 0);
  if (errInfo) {
    return ERROR;
  }

  // send the card its voltage range
  sendCommand(SEND_IF_COND, 0x000001AA);  // AA is the check pattern and 1 is
                                          // the voltage range (2.7-3.6V)
  if (errInfo) {
    return ERROR;
  }

  // trying to start the initialization process
  const uint32_t isCompleteMask = 1 << 31;
  timeout = 6;
  while (timeout--) {
    // wait(400); // TODO: add a wait
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
      error_printf("Error: An error occured during sd initialization\n");
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

  // required step
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
    error_printf("Error: Failed to set clock speed to 25000000\n");
    return response;
  }

  // select the card
  sendCommand(CARD_SELECT, relativeCardAddress);
  if (errInfo) {
    error_printf("Error: Failed to select card\n");
    return ERROR;
  }

  // wait for no data lines to be in use
  if (response = waitStatus(DATA_BUSY) != SUCCESS) return response;

  // set the block size
  *EMMC_BLKSIZECNT = 1 << 16 | BLOCKSIZE;

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
  volatile uint64_t* currentRegister = &cardConfigRegister1;
  while (timeout--) {
    // read the data
    if (*EMMC_STATUS & READ_AVAILABLE) {
      *currentRegister = *EMMC_DATA;
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
  // Read the lingering data
  while (*EMMC_STATUS & READ_AVAILABLE) {
    uint32_t data = *EMMC_DATA;
  }

  // check for bus width 4 availability
  if (cardConfigRegister1 & 1 << 10) {
    sendCommand(SET_BUS_WIDTH,
                relativeCardAddress | 0x2);  // try to set the bus width to 4
                                             // using the relative card address
    // update contol 0 to enable 4 data lines
    *EMMC_CONTROL0 |= 0x2;
  }
  if (errInfo) {
    error_printf("Error: Failed to set bus width\n");
    return ERROR;
  }

  // check if it supports SET_BLKCNT command
  canSetBlockCount = cardConfigRegister1 & SET_BLKCNT_MASK;
  if (canSetBlockCount) {
    // enable multiple block read
    printf("SD supports SET_BLKCNT command\n");
  }
  if (ccs) {
    printf("SD is SDHC and SDXC compatiable\n");
  }

  debug_printf(
      "SD is initialized with version (%d) and ccs (%d) and slot status (%d)\n",
      SDVERSION, ccs, SLOT_STATUS);
  debug_printf("CardConfigRegister1: %064b\n", cardConfigRegister1);
  debug_printf("CardConfigRegister2: %064b\n", cardConfigRegister2);
  return SUCCESS;
}

uint32_t SD::setBlockSizeCount(uint32_t startBlock, uint32_t count,
                               bool isWrite) {
  bool isSingleBlock = count == 1;
  // are we able to read multiple blocks?
  if (ccs) {
    // are we able to set the block count?
    if (isSingleBlock && canSetBlockCount) {
      sendCommand(SET_BLOCK_COUNT, count);
      if (errInfo) {
        error_printf("Error: Failed to set block count\n");
        return ERROR;
      }
    }

    // set the block size and count (count is in bits 16-31 and block size is in
    // bits 0-15)
    *EMMC_BLKSIZECNT = (count << 16) | BLOCKSIZE;

    // send the actual command to enable the read/write operation with single or
    // multiple block
    sendCommand((CMDS)(READ_SINGLE_BLOCK + (isSingleBlock * singleMultiDiff) +
                       (isWrite * readWriteDiff)),
                startBlock);
    if (errInfo) {
      error_printf("Error: Failed to read block\n");
      return ERROR;
    }
  } else {
    // can only read one block at a time
    // set the block size and count (count is in bits 16-31 and block size is in
    // bits 0-15)
    *EMMC_BLKSIZECNT = (1 << 16) | BLOCKSIZE;
  }
  return SUCCESS;
}

uint32_t SD::read(uint32_t startBlock, uint32_t count, uint8_t* buffer) {
  debug_printf("SD::read current card status: 0x%08x\n", *EMMC_STATUS);
  uint32_t response = SUCCESS;
  if (count < 1) count = 1;  // min of 1 block

  // wait for data lines to be free
  if ((response = waitStatus(DATA_BUSY)) != SUCCESS) {
    error_printf(
        "Error: Failed while waiting for EMMC data lines to be free\n");
    return response;
  }

  // cast the buffer to a uint32_t pointer for easier reading
  uint32_t* bufferPtr = (uint32_t*)buffer;

  // update block size and count
  if ((response = SD::setBlockSizeCount(startBlock, count, false)) != SUCCESS) {
    error_printf("Error: Failed to set block size and count\n");
    return response;
  }

  // try reading all the blocks
  uint32_t blockOffset = 0;

  if (ccs) {
    while (blockOffset < count) {
      // wait for interrupt indicating read is ready
      if ((response = waitInterrupt(0x20)) != SUCCESS) {
        error_printf("Error: failed to wait for sd read available\n");
        return response;
      }

      // read the data
      for (uint32_t i = 0; i < BLOCKSIZE / 4; i++) {
        *bufferPtr++ = *EMMC_DATA;
      }

      blockOffset++;
    }
  } else {
    while (blockOffset < count) {
      // if we are not supporting SDHC and SDXC we have to read one block at a
      // time
      sendCommand(READ_SINGLE_BLOCK, (startBlock + blockOffset) * BLOCKSIZE);
      if (errInfo) {
        error_printf("Error: Failed to enable read block\n");
        return ERROR;
      }

      // wait for interrupt indicating read is ready
      if ((response = waitInterrupt(0x20)) != SUCCESS) {
        error_printf("Error: failed to wait for sd read available\n");
        return response;
      }

      // read the data
      for (uint32_t i = 0; i < BLOCKSIZE / 4; i++) {
        *bufferPtr++ = *EMMC_DATA;
      }

      blockOffset++;
    }
  }

  // stop transmission
  if (count > 1 && ccs && !canSetBlockCount) {
    sendCommand(STOP_TRANS, 0);
  }

  // was there an error while reading the blocks
  if (errInfo) {
    error_printf("Error: Failed to read blocks. Only read %u blocks\n", count);
    return errInfo;
  }
  return count != blockOffset ? 0 : count * BLOCKSIZE;
}

uint32_t SD::write(uint32_t startBlock, uint32_t count, uint8_t* buffer) {
  debug_printf("SD::write current card status: 0x%08x\n", *EMMC_STATUS);
  uint32_t response = SUCCESS;
  if (count < 1) count = 1;  // min of 1 block

  // wait for data lines and write to be avaliable
  if ((response = waitStatus(DATA_BUSY | WRITE_AVAILABLE)) != SUCCESS) {
    error_printf(
        "Error: Failed while waiting for data lines to be free and write to be "
        "avaliable \n");
    return response;
  }

  // now we setup the buffer properly
  uint32_t* bufferPtr = (uint32_t*)buffer;

  // set the block size and count
  if ((response = SD::setBlockSizeCount(startBlock, count, true)) != SUCCESS) {
    error_printf("Error: Failed to set block size and count\n");
    return response;
  }

  // try reading all the blocks
  uint32_t blockOffset = 0;
  if (ccs) {
    while (blockOffset < count) {
      // wait for interrupt indicating write is ready
      if ((response = waitInterrupt(WRITE_AVAILABLE)) != SUCCESS) {
        error_printf("Error: failed to wait for sd write\n");
        return response;
      }

      // write the data
      for (uint32_t i = 0; i < BLOCKSIZE / 4; i++) {
        *EMMC_DATA = *bufferPtr++;
      }

      blockOffset++;
    }
  } else {
    while (blockOffset < count) {
      // if we are not supporting SDHC and SDXC we have to read one block at a
      // time
      sendCommand(WRITE_SINGLE, (startBlock + blockOffset) * BLOCKSIZE);
      if (errInfo) {
        error_printf("Error: Failed to enable write block\n");
        return ERROR;
      }

      // wait for interrupt indicating write is ready
      if ((response = waitInterrupt(WRITE_AVAILABLE)) != SUCCESS) {
        error_printf("Error: failed to wait for sd write\n");
        return response;
      }

      // write the data
      for (uint32_t i = 0; i < BLOCKSIZE / 4; i++) {
        *EMMC_DATA = *bufferPtr++;
      }

      blockOffset++;
    }
  }

  // stop transmission
  if (count > 1 && ccs && !canSetBlockCount) {
    sendCommand(STOP_TRANS, 0);
  }

  // was there an error?
  if (errInfo) {
    error_printf("Error: Failed to write blocks. Only wrote %u blocks\n", count);
    return errInfo;
  }
  return count != blockOffset ? 0 : count * BLOCKSIZE;
}
