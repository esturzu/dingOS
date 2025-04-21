#include "system_call.h"

#include "cores.h"
#include "event_loop.h"
#include "printf.h"
#include "process.h"

// TODO For print system calls (syscalls 2 and 3):
// - Optimize saving state (instead of saving all registers again, just select
//   the ones that actually change - for example, just x0 and x1)
// - Optimize character printing (instead of printing in a loop and using
//   printf with "%c", just add directly to the final print buffer)

uint64_t system_call_handler (uint16_t syscall_type, uint64_t* saved_state)
{
  debug_printf("Sys Call Handler\n");

  switch (syscall_type)
  {
    case 0: // Exit System Call
      {
        debug_printf("Exit System Call\n");

        uint8_t current_core = SMP::whichCore();

        Process* current_process = activeProcess[current_core];

        activeProcess[current_core] = nullptr;

        event_loop();
      }
    case 1: // Yield System Call
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
      }
    case 2: // Print Character System Call
      {
        // Convention: x0 stores the character to be printed. The system call
        // returns 1 if the character was printed successfully, and 0
        // otherwise
        printf("%c", (char) saved_state[0]);
        saved_state[0] = 1;

        uint8_t current_core = SMP::whichCore();
        Process* current_process = activeProcess[current_core];
        current_process->save_state(saved_state);
        current_process->run();
      }
    case 3: // Print String System Call
      {
        // Convention: x0 stores the pointer of data to be printed, and x1
        // stores the size of the data. The system call returns the number of
        // characters successfully printed, or -1 if some error occurred. Note
        // that it verifies that the pointer is in user space before printing.
        uint64_t pointerU64 = saved_state[0];
        constexpr uint64_t mask = 0xFFFF'0000'0000'0000;
        bool validPointer = (pointerU64 & mask) == mask;

        if (validPointer) {
          const char* pointer = (const char*) pointerU64;
          uint64_t size = saved_state[1];
          for (uint64_t i = 0; i < size; i++) printf("%c", pointer);
          saved_state[0] = size;
        } else {
          saved_state[0] = -1;
        }

        uint8_t current_core = SMP::whichCore();
        Process* current_process = activeProcess[current_core];
        current_process->save_state(saved_state);
        current_process->run();
      }
    default:
      {
        printf("Unknown System Call\n");
        while (true) {}
      }
  }

  return 0;
}
