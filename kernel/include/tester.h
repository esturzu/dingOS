#ifndef TESTER_H
#define TESTER_H

#include "cores.h"
#include "eventTests.h"
#include "heapTests.h"
#include "sdTests.h"
#include "bfsTests.h"
#include "ipc_tests.h"

void runTests() {
  eventLoopTests();

  // when running the bfs tests, you have to remake test.dd so that it isn't messed up
  // also calling delete makes heap dead hang generally
  sdTests();  
  bfsTests();
  // Must be done last until free is implemented
  heapTests();
  ipcTests();
  
}

void setupTests() {
  schedule_event([] {
    while (SMP::startedCores.load() < 4);
    runTests();
  });
}

#endif