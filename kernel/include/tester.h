#ifndef TESTER_H
#define TESTER_H

#include "eventTests.h"
#include "heapTests.h"
#include "cores.h"

void runTests() {
    eventLoopTests();
    // Must be done last until free is implemented
    heapTests();
}

void setupTests() {
    schedule_event([]{
        while(startedCores.load() < 4);
        runTests();
    });
}

#endif