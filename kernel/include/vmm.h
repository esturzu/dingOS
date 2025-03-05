// Citations
// Armv8 A-profile Architecture Reference Manual

#include "stdint.h"

#ifndef VMM_H
#define VMM_H

namespace VMM
{
  enum APTable
  {
    NoEffect = 0b00,
    PrivRW = 0b01,
    PrivR_UnprivR = 0b10,
    PrivR = 0b11
  };

  enum class PageSize : uint8_t
  {
    KiB_4 = 0,
    MiB_2,
    GiB_1
  };
}

#endif