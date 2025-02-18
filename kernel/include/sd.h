#ifndef SD_H
#define SD_H

#include "definitions.h"
#include "gpio.h"

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

 public:
  enum RESPONSE {
    TIMEOUT = -1,
    FAIL = 0,
    SUCCESS = 1,
  };
  static uint32_t SLOTISR_VER;
  static uint32_t SLOT_STATUS;
  static uint32_t SDVERSION;
  static SD::RESPONSE setClock(uint32_t freq);
  static SD::RESPONSE init();
};

#endif