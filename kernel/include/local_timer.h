#include "stdint.h"
#include "atomics.h"

#ifndef LOCAL_TIMER_H
#define LOCAL_TIMER_H

namespace LocalTimer
{
  extern void setup_timer();
  extern bool check_interrupt();
}

#endif