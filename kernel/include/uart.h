#ifndef UART_H
#define UART_H


#include "definitions.h"
#include "gpio.h"
#include "stdint.h"

// for the PL011 UART

extern "C" void uart_putc(char c);

extern "C" char uart_getc();

extern bool uart_hasInput();

#endif