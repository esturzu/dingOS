// Citations
// https://developer.arm.com/documentation/101107/0000/Programmers-model/UART-registers
// https://wiki.osdev.org/Detecting_Raspberry_Pi_Board

#ifndef UART_H
#define UART_H

#include "stdint.h"

class UART_Registers
{
  static constexpr uint64_t MMIO_BASE = 0xFE000000;

  static constexpr uint64_t UART0_OFFSET = 0x201000;

  static constexpr uint64_t DATA_REG_OFFSET = 0x0000;
  static constexpr uint64_t UART0RSR_OFFSET = 0x0004;
  static constexpr uint64_t FLAG_REG_OFFSET = 0x0018;
  static constexpr uint64_t UART0ILPR_OFFSET = 0x0020;
  static constexpr uint64_t UART0IBRD_OFFSET = 0x0024;
  static constexpr uint64_t UART0FBRD_OFFSET = 0x0028;
  static constexpr uint64_t UART0LCR_H_OFFSET = 0x002C;
  static constexpr uint64_t UART0CR_OFFSET = 0x0030;
  static constexpr uint64_t UART0IFLS_OFFSET = 0x0034;
  static constexpr uint64_t UART0IMSC_OFFSET = 0x0038;
  static constexpr uint64_t UART0RIS_OFFSET = 0x003C;
  static constexpr uint64_t UART0MIS_OFFSET = 0x0040;
  static constexpr uint64_t UART0ICR_OFFSET = 0x0044;
  static constexpr uint64_t UART0DMACR_OFFSET = 0x0048;

public:
  
  UART_Registers() {};

};

#endif