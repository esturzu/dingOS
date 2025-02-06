#ifndef TESTER_H
#define TESTER_H

#include "eventTests.h"
#include "heapTests.h"

void runTests() {
    eventLoopTests();

    // Heap tests must be last until we have free
    heapTests();
}

#endif