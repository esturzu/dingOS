#ifndef CORES_H
#define CORES_H

#include "atomics.h"
#include "stdint.h"

namespace SMP {
extern Atomic<int> startedCores;

extern "C" void bootCores();

extern uint8_t whichCore();
}  // namespace SMP

#endif