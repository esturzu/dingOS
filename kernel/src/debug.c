#include "debug.h"

#include "uart.h"

void debug_print(const char* string, ...)
{
  char* pos = (char*) string;
  while (*pos != '\0')
  {
    uart_putc(*pos);
    pos += 1;
  }
}