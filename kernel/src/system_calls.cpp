#include "system_call.h"

#include "cores.h"
#include "event_loop.h"
#include "printf.h"
#include "process.h"

uint64_t system_call_handler (uint16_t syscall_type, uint64_t* saved_state)
{
  debug_printf("Sys Call Handler\n");

  switch (syscall_type)
  {
    case 0: // Yield System Call
      {
        debug_printf("Yield System Call\n");

        uint8_t current_core = SMP::whichCore();

        Process* current_process = activeProcess[current_core];

        current_process->save_state(saved_state);

        activeProcess[current_core] = nullptr;

        __asm__ volatile("dmb sy" ::: "memory");

        schedule_event([current_process](){
          current_process->run();
        });

        event_loop();

        break;
      }
    default:
      {
        printf("Unknown System Call\n");
        while (true) {}
      }
  }

  return 0;
}