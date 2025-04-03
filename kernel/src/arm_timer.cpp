// Citations 
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Core3Interrupt

#include "arm_timer.h"

#include "interrupts.h"
#include "machine.h"
#include "printf.h"

void ARMTimer::setup_timer()
{
  volatile uint32_t* local_interrupt_routing_register =
      (volatile uint32_t*)(system_timer_base_address + local_interrupt_routing_offset);
  
  volatile uint32_t* timer_control_status_register =
    (volatile uint32_t*)(system_timer_base_address + timer_control_status_offset);
  
  volatile uint32_t* timer_clear_reload_register =
    (volatile uint32_t*)(system_timer_base_address + timer_clear_reload_offset);
  
  volatile uint32_t* core_0_timer_int_control_register =
    (volatile uint32_t*)(system_timer_base_address + core_0_timer_int_control_offset);

  *local_interrupt_routing_register = 0;

  *timer_control_status_register = (1 << 29) | (1 << 28) | 2000000;

  *timer_clear_reload_register = (1 << 31) | (1 << 30);

  *core_0_timer_int_control_register = (1 << 1);  

  printf("Here\n");

  while (true)
  {
    uint64_t x = 0;
    while (true)
    {
      x ++;
      if (x > 100000000)
      {
        printf("Heartbeat\n");
        x = 0;
      }
    }
  }

  // Interrupts::Enable_Base(0);

  // set_CNTKCTL_EL1(0); // Trap EL0 Physical Timer Accesses
  // set_CNTP_CTL_EL0(0b1); // Timer Interrupt Masked by IMASK bit | Timer Enabled
  
  // printf("%ld\n", get_CNTP_TVAL_EL0());
  
  // set_CNTP_TVAL_EL0(5000);

  // __asm__ volatile("dsb sy");
  
  // volatile uint32_t* core_0_interrupt_control_register =
  //   (volatile uint32_t*)(system_timer_base_address + core_0_interrupt_control);
  
  // *core_0_interrupt_control_register = 0b1;

  // printf("Here\n");

  // while (1)
  // {
  //   printf("x");
  //   // if (get_CNTP_CTL_EL0() & 0b100)
  //   //   printf("%ld\n", get_CNTP_CVAL_EL0());
    
  //   // printf("Pending %lx\n", Interrupts::get_basic_pending_register());
  // }

  // volatile uint32_t* control_register =
  //     (volatile uint32_t*)(system_timer_base_address + control_offset);
  
  // volatile uint32_t* prescaler_register =
  //   (volatile uint32_t*)(system_timer_base_address + core_timer_prescaler);
  
  // volatile uint32_t* ls_register =
  //   (volatile uint32_t*)(system_timer_base_address + core_timer_access_ls);
  
  // volatile uint32_t* ms_register =
  //   (volatile uint32_t*)(system_timer_base_address + core_timer_access_ms);

  // printf("Here\n");

  // *control_register = 0b100000000;

  // __asm__ volatile ("isb" ::: "memory");

  // printf("Not Here\n");

  // *prescaler_register = 0xFFFFFFFF;

  // __asm__ volatile ("isb" ::: "memory");

  // *ls_register = 0xFFFFFFFF;
  // *ms_register = 0xFFFFFFFF;

  // __asm__ volatile ("isb" ::: "memory");

  // printf("%u\n", *ls_register);

  // while(true)
  // {
  //   // printf("%u\n", *ls_register);
  // }
}