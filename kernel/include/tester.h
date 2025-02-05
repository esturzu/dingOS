#ifndef TESTER_H
#define TESTER_H

#include "eventTests.h"
#include "heapTests.h"

void runTests() {
    eventLoopTests();
    heapTests();
}

#endif