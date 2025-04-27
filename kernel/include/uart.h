#ifndef UART_H
#define UART_H

#include "stdint.h"

extern "C" void uart_init(uint64_t base_address);
extern "C" void uart_putc(char c);

#endif