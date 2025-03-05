/*
 * https://developer.arm.com/documentation/100417/0000/programmers-model/uart-registers
 * https://ultibo.org/wiki/Unit_BCM2837
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// Stack
#define STACK_SIZE 8192

// Peripherals
#define PERIPHERALS_BASE 0xFFFF00003F000000
#define GPIO_BASE 0xFFFF000000200000
#define UART0_BASE 0xFFFF00003F201000  // PERIPHERALS_BASE + 0x201000

// HEAP
#define HEAP_START ((size_t)&_end)
#define HEAP_SIZE 0x100000  // Example: 1 MB heap size
#define HEAP_END (HEAP_START + HEAP_SIZE)

// TOOLS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CLEAN_EL(x) (((x) >> 2) & 0b11)
#define STRING_EL(x) ((x) == 0b0000 ? "EL0" : ((x) == 0b0100 ? "EL1" : ((x) == 0b1000 ? "EL2" : ((x) == 0b1100 ? "EL3" : "Unknown"))))

#endif