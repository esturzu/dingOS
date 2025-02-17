#include "kernel.h"
#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
#include "event_loop.h"
#include "heap.h"
#include "printf.h"
#include "stdint.h"
#include "uart.h"
#include "testFramework.h"

// Atomic flag to indicate when the test is complete
Atomic<int> testComplete;  // Using int as a substitute for boolean (0 = false, 1 = true)

// Function containing all the event loop test logic
void runEventLoopTests() {
    initTests("Event Loop Tests");

    // --- Test 1: Basic event scheduling ---
    Atomic<int> total = Atomic<int>(0);

    // Schedule 10 events that increment the `total` counter
    for (int i = 0; i < 10; i++) {
        schedule_event([&] { total.add_fetch(1); });
    }

    // Wait for all events to finish
    while (total.load() < 10) {}

    // Verify that all events were processed successfully
    testsResult("Basic event scheduling", total.load() == 10);

    Debug::printf("Event Loop Tests completed.\n");

    // Signal that the test is complete by setting the flag
    schedule_event([] { testComplete.store(1); });
}

// Schedule the test to run within the event loop
void setupEventLoopTestEvent() {
    schedule_event([] {
        while (SMP::startedCores.load() < 4);  // UPDATED REFERENCE
        runEventLoopTests();
    });
}

extern "C" void kernelMain() {
    CRTI::_init();

    // System setup
    heap_init();
    init_event_loop();

    Debug::printf("DingOS is Booting!\n");

    // Boot all cores
    SMP::bootCores();  // UPDATED REFERENCE

    // Schedule the event loop tests
    setupEventLoopTestEvent();

    // Run the event loop until the test is complete
    while (testComplete.load() == 0) {
        Debug::printf("about to call event_loop\n");
        event_loop();
    }

    Debug::printf("Test execution complete. Exiting kernelMain.\n");
    
    // Halt the system after the event loop finishes
    while (1);
}
