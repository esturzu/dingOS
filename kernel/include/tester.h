#ifndef TESTER_H
#define TESTER_H

#include "cores.h"
#include "eventTests.h"
#include "heapTests.h"
#include "sdTests.h"
#include "elfTests.h"
#include "primitives_tests.h"

void runTests() {
  elfTests();
  eventLoopTests();

  // when running the bfs tests, you have to remake test.dd so that it isn't messed up
  // also calling delete makes heap dead hang generally
  // sdTests();  
  // Must be done last until free is implemented
  heapTests();
  primitives_tests();
}

void setupTests() {
  schedule_event([] {
    while (SMP::startedCores.load() < 4);
    runTests();
  });
}

#endif
