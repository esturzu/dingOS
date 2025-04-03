#include "printf.h"
#include "system_call.h"

uint64_t system_call_handler (uint16_t syscall_type, uint64_t* saved_state)
{
  switch (syscall_type)
  {
    case 0: // Yield System Call
      {
        printf("Here\n");
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