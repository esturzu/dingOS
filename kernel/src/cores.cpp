#include "cores.h"

#include "definitions.h"
#include "devices.h"
#include "event_loop.h"
#include "machine.h"
#include "printf.h"
#include "stdint.h"
#include "vmm.h"

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
  debug_printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  VMM::init_core();

  event_loop();
}

extern "C" void initCore2() {
  debug_printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  VMM::init_core();

  event_loop();
}

extern "C" void initCore3() {
  debug_printf("Core %d! %s\n", whichCore(), STRING_EL(get_CurrentEL()));
  startedCores.add_fetch(1);

  VMM::init_core();

  event_loop();
}

void bootCores() {
  startedCores.add_fetch(1);
  // Boot other cores
  void* core_1_device = Devices::get_device("/cpus/cpu@1");
  Devices::DTB_Property cpu_1_release_addr = Devices::get_device_property(core_1_device, "cpu-release-addr");
  void* core_2_device = Devices::get_device("/cpus/cpu@2");
  Devices::DTB_Property cpu_2_release_addr = Devices::get_device_property(core_2_device, "cpu-release-addr");
  void* core_3_device = Devices::get_device("/cpus/cpu@3");
  Devices::DTB_Property cpu_3_release_addr = Devices::get_device_property(core_3_device, "cpu-release-addr");

  *(reinterpret_cast<volatile uint64_t*>(VMM::phys_to_kernel_ptr(cpu_1_release_addr.to_uint64_t()))) = VMM::kernel_to_phys_ptr((uint64_t)&_start_core1);
  *(reinterpret_cast<volatile uint64_t*>(VMM::phys_to_kernel_ptr(cpu_2_release_addr.to_uint64_t()))) = VMM::kernel_to_phys_ptr((uint64_t)&_start_core2);
  *(reinterpret_cast<volatile uint64_t*>(VMM::phys_to_kernel_ptr(cpu_3_release_addr.to_uint64_t()))) = VMM::kernel_to_phys_ptr((uint64_t)&_start_core3);
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