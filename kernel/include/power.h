#ifndef POWER_H
#define POWER_H

#include "gpio.h"
#include "interrupts.h"
#include "printf.h"
#include "stdint.h"

// https://github.com/bztsrc/raspi3-tutorial/blob/master/08_power/power.c
// trying to understand what they are doing but its kinda annoying and not
// properly documented

#define MMIO_BASE 0x3F000000
#define PM_RSTC ((volatile unsigned int*)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile unsigned int*)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int*)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020

void shutdown() {
  debug_printf("powering off\n");
  // first disable all interrupts
  Interrupts::disable_fiq_interrupt();
  Interrupts::Disable_All_IRQ();

  // power off all devices
  // ?

  // disable all GPIO
  // this is missing turning off all the fsel
  GPIO::set_pull_register(GPIO::PUD::OFF);
  GPIO::set_clock(0, 0xFFFFFFFF);
  GPIO::set_clock(1, 0xFFFFFFFF);
  // supposed to wait here
  GPIO::set_clock(0, 0);
  GPIO::set_clock(1, 0);

  // weird watchdog stuff
  uint32_t r = *PM_RSTS;
  r &= 0x555;
  r |= 0x555;
  *PM_RSTS = PM_WDOG_MAGIC | r;
  *PM_WDOG = PM_WDOG_MAGIC | 10;
  *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

#endif