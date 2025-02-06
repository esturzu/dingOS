#ifndef CORES_H
#define CORES_H

#include "atomics.h"

extern Atomic<int> startedCores;

extern "C" void bootCores();

#endif