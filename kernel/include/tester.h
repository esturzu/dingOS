#ifndef TESTER_H
#define TESTER_H

#include "cores.h"
#include "eventTests.h"
#include "hashmapTests.h"
#include "heapTests.h"

void runTests() {
  eventLoopTests();
  hashmapTests();

  // Must be done last until free is implemented
  // heapTests();
}

void setupTests() {
  schedule_event([] {
    while (SMP::startedCores.load() < 4)
      ;
    runTests();
  });
}

#endif
