/*
 * https://developer.arm.com/documentation/100417/0000/programmers-model/uart-registers
 * https://ultibo.org/wiki/Unit_BCM2837
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// Stack
#define STACK_SIZE 8192

// Peripherals
#define PERIPHERALS_BASE = 0x3F000000
#define UART0_DATA_PHYS 0x3f201000     // PERIPHERALS_BASE + 0x201000
#define UART0_FLAG_PHYS 0x3f201018     // UART0_DATA_PHYS + 0x18
#define UART0_CONTROL_PHYS 0x3f201030  // UART0_DATA_PHYS + 0x30

// HEAP
#define HEAP_START ((size_t)&_end)
#define HEAP_SIZE 0x100000  // Example: 1 MB heap size
#define HEAP_END (HEAP_START + HEAP_SIZE)

#endif