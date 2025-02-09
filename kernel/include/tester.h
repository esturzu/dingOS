#ifndef TESTER_H
#define TESTER_H

#include "cores.h"
#include "eventTests.h"
#include "heapTests.h"

void runTests() {
  eventLoopTests();
  // Must be done last until free is implemented
  heapTests();
}

void setupTests() {
  schedule_event([] {
    while (startedCores.load() < 4);
    runTests();
  });
}

#endif