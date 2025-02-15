#include "cores.h"

#include "definitions.h"
#include "event_loop.h"
#include "machine.h"
#include "printf.h"
#include "stdint.h"

/**
 * @brief Symmetric Multiprocessing (SMP) aka multicore support
 * 
 */
namespace SMP {
Atomic<int> startedCores = Atomic<int>(0);

uint8_t stack0[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack0_top;
uint8_t* stack0_top = (stack0 + STACK_SIZE);

uint8_t stack1[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack1_top;
uint8_t* stack1_top = (stack1 + STACK_SIZE);

uint8_t stack2[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack2_top;
uint8_t* stack2_top = (stack2 + STACK_SIZE);

uint8_t stack3[STACK_SIZE] __attribute__((aligned(16)));
extern "C" uint8_t* stack3_top;
uint8_t* stack3_top = (stack3 + STACK_SIZE);

extern "C" void _start_core1();
extern "C" void _start_core2();
extern "C" void _start_core3();

extern "C" void initCore1() {
  Debug::printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  event_loop();
}

extern "C" void initCore2() {
  Debug::printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  event_loop();
}

extern "C" void initCore3() {
  Debug::printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  event_loop();
}

void bootCores() {
  startedCores.add_fetch(1);
  // Boot other cores
  uint64_t* core_wakeup_base = (uint64_t*)216;
  *(core_wakeup_base + 1) = (uint64_t)&_start_core1;
  *(core_wakeup_base + 2) = (uint64_t)&_start_core2;
  *(core_wakeup_base + 3) = (uint64_t)&_start_core3;
}

/**
 * @brief Gets the core number of core executing this function
 *
 * @return uint8_t  Core number
 */
uint8_t whichCore() {
  uint64_t mpidr;
  // Grabbing the Multiprocessor Affinity Register
  asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
  return mpidr & 0xFF;
}
}  // namespace SMP