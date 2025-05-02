#include "machine.h"
#include "stdint.h"
#include "system_timer.h"

void* el1_vector_table = nullptr;

uint32_t ticks_ms() {
  return (uint32_t)(SystemTimer::get_free_running_counter_value() / 1000ULL);
}

void delay_ms(uint32_t ms) {
  uint32_t start = ticks_ms();
  while ((ticks_ms() - start) < ms) {
    asm volatile("yield");
  }
}
