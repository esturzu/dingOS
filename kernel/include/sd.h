#ifndef SD_H
#define SD_H

#include "definitions.h"
#include "gpio.h"

/**
 * @brief SD card interface
 *
 * Resources:
 *  +
 * https://github.com/bztsrc/raspi3-tutorial/blob/master/15_writesector/sd.c#L160
 *  + https://chlazza.nfshost.com/sdcardinfo.html
 *  + https://myembeddedlinux.blogspot.com/2017/03/interfacing-emmc.html
 *  + Part 1 Physical Layer Simplified Specification Ver9.10
 *     + Figure 4-13: SD Memory Card State Diagram (data transfer mode)
 *
 */
class SD {
  // SD response types
  static constexpr uint32_t SD_APP_CMD_ENABLED = 0x00000020;

  // Interrupt masks
  static constexpr uint32_t INT_ERROR_MASK = 0x017f3f00;
  static constexpr uint32_t INT_TIMEOUT_MASK = 0x00110000;

  // Status masks
  static constexpr uint32_t ERRORS_MASK = 0xfff9c004;
  static constexpr uint32_t READ_AVAILABLE = 0x00000800;
  static constexpr uint32_t WRITE_AVAILABLE = 0x00000400;
  static constexpr uint32_t DATA_BUSY = 0x00000002;
  static constexpr uint32_t CMD_BUSY = 0x00000001;

  // SCR masks
  static constexpr uint32_t CCS_MASK = 1 << 30;
  static constexpr uint32_t SET_BLKCNT_MASK = 1 << 25;

  // OP_COND flags
  static uint32_t const OP_COND_NOT_SDSC = 1 << 30;
  static uint32_t const VOLTAGE_WINDOW = 0x1FF << 15;
  static uint32_t constexpr OP_COND_FULL =
      OP_COND_NOT_SDSC   // yes for for both types (SDHC and SDXC)
      | 1 << 28          // yes for SDXC Maximum Performance
      | 1 << 24          // switch to 1.8V signal voltage
      | VOLTAGE_WINDOW;  // Voltage window 2.7-3.6V

  // SD emmc registers
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

  // Etc
  static constexpr uint32_t singleMultiDiff = 0x01000022;
  static constexpr uint32_t readWriteDiff = 0x006FFFFF0;

  /**
   * @brief Sets the block size and count for reading or writing
   *
   * @param startBlock  First block to start reading or writing from
   * @param count       Number of blocks to read or write
   * @param isRead      True if reading, false if writing
   * @return uint32_t   SUCCESS if successful, ERROR otherwise
   */
  static uint32_t setBlockSizeCount(uint32_t startBlock, uint32_t count,
                                    bool isRead);

 public:
  enum RESPONSE {
    ERROR = -2,
    TIMEOUT = -1,
    FAIL = 0,
    SUCCESS = 1,
  };

  enum CMDS {
    GO_IDLE = 0x00000000,        // go idle state
    ALL_SEND_CID = 0x02010000,   // ask all cards to send card identification
    SEND_REL_ADDR = 0x03020000,  // send relative address
    CARD_SELECT = 0x07030000,    // select/deselect card
    SEND_IF_COND = 0x08020000,   // send interface condition
    STOP_TRANS = 0x0C030000,     // stop transmission
    READ_SINGLE_BLOCK = 0x11220010,    // CMD17: read single block
    READ_MULTIPLE_BLOCK = 0x12220032,  // CMD18: read multiple blocks
    SET_BLOCK_COUNT = 0x17020000,      // CMD23: set block count
    WRITE_SINGLE = 0x18220000,         // write single block
    WRITE_MULTI = 0x19220022,          // write multiple blocks
    APP_CMD = 0x37000000,              // application specific command
    APP_CMD_RCA =
        0x37010000,  // application specific command with relative card address
    SET_BUS_WIDTH = 0x06020000,  // set bus width
    SEND_OP_COND = 0x29020000,   // ACMD41: send operation condition
    SEND_SCR = 0x33220010        // send SD Card Configuration Register
  };

  static const uint32_t BLOCKSIZE = 512;

  // SD card static information
  static bool ccs;
  static bool canSetBlockCount;
  static uint32_t SLOTISR_VER;
  static uint32_t SLOT_STATUS;
  static uint32_t SDVERSION;

  // SD card configuration registers
  static uint64_t cardConfigRegister1;
  static uint64_t cardConfigRegister2;
  static uint32_t relativeCardAddress;

  // For passing debugging information
  static uint32_t errInfo;

  /**
   * @brief Waits for an interrupt to be set for the given mask or a
   * timeout/error
   *
   * @param mask              mask of the interrupt to wait for
   * @param timeout           number of times to check before timing out
   * @return SD::RESPONSE     result of the wait
   */
  static SD::RESPONSE waitInterrupt(uint32_t mask, uint32_t timeout = 1000000);

  /**
   * @brief Waits for the status register to have 0s in the bits specified by
   * mask
   *
   * @param mask              what bits to wait for 0 from status register
   * @param timeout           number of times to check before timing out
   * @return SD::RESPONSE     result of the wait
   */
  static SD::RESPONSE waitStatus(uint32_t mask, uint32_t timeout = 1000000);

  /**
   * @brief Sends a command to the SD card
   * Note! RELATIVE_CARD_ADDRESS doesnt not return normaly. the rca is returned
   * in the response and the error info is returned in the errInfo variable with
   * the bits in the correct place
   *
   * @param cmd               command to send
   * @param arg               arguments to send with the command
   * @return SD::RESPONSE     result of the command
   */
  static uint32_t sendCommand(SD::CMDS cmd, uint32_t arg);

  /**
   * @brief Set the clock frequency for the SD card
   *
   * @param freq          frequency to set the clock to
   * @return SD::RESPONSE result of the command
   */
  static SD::RESPONSE setClock(uint32_t freq);

  /**
   * @brief Initialize the eMMC card part of the SD card
   *
   */
  static void eMMCinit();

  /**
   * @brief Initialize the SD card
   *
   * @return uint32_t   result of the initialization (matches with RESPONSE
   * enum)
   */
  static uint32_t init();

  /**
   * @brief Read from the SD card
   *
   * @param startBlock    block to start reading from
   * @param count         number of blocks to read
   * @param buffer        buffer to read the data into
   * @return uint32_t     number of bytes read
   */
  static uint32_t read(uint32_t startBlock, uint32_t count, uint8_t* buffer);

  /**
   * @brief Write to the SD card
   *
   * @param startBlock    block to start writing to
   * @param count         number of blocks to write
   * @param buffer        buffer to write to the SD card
   * @return uint32_t     number of bytes written
   */
  static uint32_t write(uint32_t startBlock, uint32_t count, uint8_t* buffer);
};

#endif