#ifndef TESTER_H
#define TESTER_H

#include "cores.h"
#include "eventTests.h"
#include "heapTests.h"
#include "sdTests.h"
#include "elfTests.h"

void runTests() {
  elfTests();
  eventLoopTests();

  sdTests();
  // Must be done last until free is implemented
  heapTests();
}

void setupTests() {
  schedule_event([] {
    while (SMP::startedCores.load() < 4);
    runTests();
  });
}

#endif
