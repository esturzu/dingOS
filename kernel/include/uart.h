#ifndef UART_H
#define UART_H

extern "C" void uart_putc(char c);

void uart_init(uint64_t base_address);

#endif