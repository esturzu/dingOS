// Citations 
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Core3Interrupt

#include "local_timer.h"

#include "cores.h"
#include "interrupts.h"
#include "machine.h"
#include "printf.h"

namespace LocalTimer
{
  Atomic<int> interrupting_core = Atomic<int>(0);

  void setup_timer()
  {
    while (SMP::startedCores.load() < 4) {} // Wait Until All Cores Have Booted
    
    uint64_t system_timer_base_address = 0xffff000040000000;

    uint64_t local_interrupt_routing_offset = 0x24;

    uint64_t timer_control_status_offset = 0x34;
    uint64_t timer_clear_reload_offset = 0x38;

    uint64_t core_0_timer_int_control_offset = 0x40;
    uint64_t core_1_timer_int_control_offset = 0x44;
    uint64_t core_2_timer_int_control_offset = 0x48;
    uint64_t core_3_timer_int_control_offset = 0x4c;

    volatile uint32_t* local_interrupt_routing_register =
        (volatile uint32_t*)(system_timer_base_address + local_interrupt_routing_offset);
    
    volatile uint32_t* timer_control_status_register =
      (volatile uint32_t*)(system_timer_base_address + timer_control_status_offset);
    
    volatile uint32_t* timer_clear_reload_register =
      (volatile uint32_t*)(system_timer_base_address + timer_clear_reload_offset);
    
    volatile uint32_t* core_0_timer_int_control_register =
      (volatile uint32_t*)(system_timer_base_address + core_0_timer_int_control_offset);

    volatile uint32_t* core_1_timer_int_control_register =
      (volatile uint32_t*)(system_timer_base_address + core_1_timer_int_control_offset);
    
    volatile uint32_t* core_2_timer_int_control_register =
      (volatile uint32_t*)(system_timer_base_address + core_2_timer_int_control_offset);

    volatile uint32_t* core_3_timer_int_control_register =
      (volatile uint32_t*)(system_timer_base_address + core_3_timer_int_control_offset);

    *local_interrupt_routing_register = 0;

    *timer_control_status_register = (1 << 29) | (1 << 28) | 2000000;

    *timer_clear_reload_register = (1 << 31) | (1 << 30);

    *core_0_timer_int_control_register = (1 << 1);
    *core_1_timer_int_control_register = (1 << 1);  
    *core_2_timer_int_control_register = (1 << 1);  
    *core_3_timer_int_control_register = (1 << 1); 

    debug_printf("Finished Initializing Local Timer\n");
  }

  bool check_interrupt()
  {
    uint64_t system_timer_base_address = 0xffff000040000000;

    uint64_t local_interrupt_routing_offset = 0x24;
    uint64_t local_timer_control_and_status_offset = 0x34;
    uint64_t timer_clear_reload_offset = 0x38;

    volatile uint32_t* local_timer_control_and_status_register =
        (volatile uint32_t*)(system_timer_base_address + local_timer_control_and_status_offset);

    uint8_t current_core = SMP::whichCore();
  
    if ((*local_timer_control_and_status_register & (1 << 31)) && current_core == interrupting_core.load())
    {
      volatile uint32_t* local_interrupt_routing_register =
        (volatile uint32_t*)(system_timer_base_address + local_interrupt_routing_offset);

      volatile uint32_t* timer_clear_reload_register =
        (volatile uint32_t*)(system_timer_base_address + timer_clear_reload_offset);

      interrupting_core.store((current_core + 1) % 4);

      *local_interrupt_routing_register = (current_core + 1) % 4;

      *timer_clear_reload_register = (1 << 31) | (1 << 30);

      return true;
    }

    return false;
  }
}