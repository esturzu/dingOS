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
    while (total.load() < 10) {
        // Optionally, you can add a debug message to monitor progress
        // Debug::printf("Waiting for events to finish...\n");
    }

    // Verify that all events were processed successfully
    testsResult("Basic event scheduling", total.load() == 10);

    Debug::printf("Event Loop Tests completed.\n");
}

// Schedule the test to run within the event loop
void setupEventLoopTestEvent() {
    schedule_event([] {
        // Wait for all cores to boot
        while (startedCores.load() < 4);

        // Run the tests after cores are synchronized
        runEventLoopTests();
    });
}

extern "C" void kernelMain() {
    CRTI::_init();

    // System setup
    heap_init();
    init_event_loop();
    init_uart();

    Debug::printf("DingOS is Booting!\n");

    // Boot all cores
    bootCores();

    // Schedule the event loop tests as an event
    setupEventLoopTestEvent();

    // Enter the event loop to handle core synchronization and events
    event_loop();

    // Halt the system after the event loop
    while (1);
}
