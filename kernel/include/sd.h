#ifndef SD_H
#define SD_H

#include "definitions.h"
#include "gpio.h"

#define ERRORS_MASK 0xfff9c004
#define CCS_MASK 1 << 30
#define SET_BLKCNT_MASK 1 << 25

// SD response types
#define SD_APP_CMD_ENABLED 0x00000020

// interrupt masks
#define INT_ERROR_MASK 0x017f3f00
#define INT_TIMEOUT_MASK 0x00110000

#define READ_AVAILABLE 0x00000800
#define WRITE_AVAILABLE 0x00000400
#define DATA_BUSY 0x00000002
#define CMD_BUSY 0x00000001

class SD {
  static volatile uint32_t* const EMMC_BASE;
  static volatile uint32_t* const EMMC_ARG2;
  static volatile uint32_t* const EMMC_BLKSIZECNT;
  static volatile uint32_t* const EMMC_ARG1;
  static volatile uint32_t* const EMMC_CMDTM;
  static volatile uint32_t* const EMMC_RESP0;
  static volatile uint32_t* const EMMC_RESP1;
  static volatile uint32_t* const EMMC_RESP2;
  static volatile uint32_t* const EMMC_RESP3;
  static volatile uint32_t* const EMMC_DATA;
  static volatile uint32_t* const EMMC_STATUS;
  static volatile uint32_t* const EMMC_CONTROL0;
  static volatile uint32_t* const EMMC_CONTROL1;
  static volatile uint32_t* const EMMC_INTERRUPT;
  static volatile uint32_t* const EMMC_IRPT_MASK;
  static volatile uint32_t* const EMMC_IPRT_EN;
  static volatile uint32_t* const EMMC_CONTROL2;
  static volatile uint32_t* const EMMC_FORCE_IRPT;
  static volatile uint32_t* const EMMC_BOOT_TIMEOUT;
  static volatile uint32_t* const EMMC_DBG_SEL;
  static volatile uint32_t* const EMMC_EXRDFIFIO_CFG;
  static volatile uint32_t* const EMMC_EXRDFIFO_EN;
  static volatile uint32_t* const EMMC_TUNE_STEP;
  static volatile uint32_t* const EMMC_TUNE_STEP_STD;
  static volatile uint32_t* const EMMC_TUNE_STEP_DDR;
  static volatile uint32_t* const EMMC_INT_SPI;
  static volatile uint32_t* const EMMC_SLOTISR_VER;

  static uint32_t const OP_COND_NOT_SDSC = 1 << 30;
  static uint32_t const VOLTAGE_WINDOW = 0x1FF << 15;
  static uint32_t constexpr OP_COND_FULL =
      OP_COND_NOT_SDSC   // yes for for both types (SDHC and SDXC)
      | 1 << 28          // yes for SDXC Maximum Performance
      | 1 << 24          // switch to 1.8V signal voltage
      | VOLTAGE_WINDOW;  // Voltage window 2.7-3.6V

  static uint32_t setBlockSizeCount(uint32_t startBlock, uint32_t count, bool isRead);

 public:
  static const uint32_t BLOCKSIZE = 512;
  enum RESPONSE {
    ERROR = -2,
    TIMEOUT = -1,
    FAIL = 0,
    SUCCESS = 1,
  };

  // used same similar as
  // https://github.com/bztsrc/raspi3-tutorial/blob/master/15_writesector/sd.c#L160
  // https://chlazza.nfshost.com/sdcardinfo.html
  // https://myembeddedlinux.blogspot.com/2017/03/interfacing-emmc.html

  enum CMDS {
    GO_IDLE = 0x00000000,        // go idle state
    ALL_SEND_CID = 0x02010000,   // ask all cards to send card identification
    SEND_REL_ADDR = 0x03020000,  // send relative address
    CARD_SELECT = 0x07030000,    // select/deselect card
    SEND_IF_COND = 0x08020000,   // send interface condition
    STOP_TRANS = 0x0C030000,     // stop transmission
    READ_SINGLE = 0x11220010,    // read single block
    READ_MULTI = 0x12220032,     // read multiple blocks
    SET_BLOCKCNT = 0x17020000,   // set block count
    WRITE_SINGLE = 0x18220000,   // write single block
    WRITE_MULTI = 0x19220022,    // write multiple blocks
    APP_CMD = 0x37000000,        // application specific command
    APP_CMD_RCA =
        0x37010000,  // application specific command with relative card address
    SET_BUS_WIDTH = 0x06020000,  // set bus width
    SEND_OP_COND = 0x29020000,   // send operation condition
    SEND_SCR = 0x33220010        // send SD Card Configuration Register
  };

  static uint32_t SLOTISR_VER;
  static uint32_t SLOT_STATUS;
  static uint32_t SDVERSION;

  static volatile uint64_t* cardConfigRegister1;
  static volatile uint64_t* cardConfigRegister2;
  static uint32_t relativeCardAddress;
  static uint32_t errInfo;

  static SD::RESPONSE waitInterrupt(uint32_t mask, uint32_t timeout = 1000000);
  static SD::RESPONSE waitStatus(uint32_t mask, uint32_t timeout = 1000000);
  static uint32_t sendCommand(SD::CMDS cmd, uint32_t arg);
  static SD::RESPONSE setClock(uint32_t freq);
  static uint32_t init();
  int SET_BLKCNT_CONFIG_MASK();
  int NewFunction();
  static uint32_t read(uint32_t startBlock, uint32_t count, uint8_t* buffer);
  static uint32_t write(uint32_t startBlock, uint32_t count, uint8_t* buffer);
};

#endif