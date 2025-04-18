#include "system_call.h"
#include "cores.h"
#include "event_loop.h"
#include "printf.h"
#include "process.h"
#include "ext2.h"

#define USER_MASK 0xFFFF'0000'0000'0000
#define IN_USER(ptr) ((((uint64_t) (ptr)) & USER_MASK) == USER_MASK)

// TODO For print system calls (syscalls 2 and 3):
// - Optimize saving state (instead of saving all registers again, just select
//   the ones that actually change - for example, just x0 and x1)
// - Optimize character printing (instead of printing in a loop and using
//   printf with "%c", just add directly to the final print buffer)

int fork() {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int join(int pid) {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int getpid() {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int open(const char* filename) {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int close(int fd) {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int read(char* buffer, int count, int fd) {
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

int write(const char* buffer, int count, int fd) {
  if (fd != 1) return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
  if (!IN_USER(buffer)) return (int) SystemCallErrorCode::INVALID_POINTER;
  for (int i = 0; i < count; i++) printf("%c", buffer[i]);
  return count;
}

int exec(const char* filename, int argc, const char** argv) {
  if (!IN_USER(filename)) return (int) SystemCallErrorCode::INVALID_POINTER;
  Node* output = find_from_abs_path(filename);
  if (output == nullptr) return (int) SystemCallErrorCode::FILE_NOT_FOUND;
  return (int) SystemCallErrorCode::NOT_IMPLEMENTED;
}

void current_process_return(uint64_t* saved_state, int result) {
  saved_state[0] = result;
  Process* current_process = activeProcess[SMP::whichCore()];
  current_process->save_state(saved_state);
  current_process->run();
}

uint64_t system_call_handler(uint16_t syscall_type, uint64_t* saved_state) {
  debug_printf("Sys Call Handler\n");
  switch (syscall_type) {
    // Exit System Call
    case 0x00: {
      debug_printf("Exit System Call\n");
      uint8_t current_core = SMP::whichCore();
      Process* current_process = activeProcess[current_core];
      delete current_process;
      activeProcess[current_core] = nullptr;
      event_loop();
      break;
    }

    // Yield System Call
    case 0x01: {
      debug_printf("Yield System Call\n");
      uint8_t current_core = SMP::whichCore();
      Process* current_process = activeProcess[current_core];
      current_process->save_state(saved_state);
      activeProcess[current_core] = nullptr;
      __asm__ volatile("dmb sy" ::: "memory");
      schedule_event([current_process] () { current_process->run(); });
      event_loop();
      break;
    }

    case 0x02: {
      current_process_return(saved_state, fork());
      break;
    }

    case 0x03: {
      current_process_return(saved_state, join((int) saved_state[0]));
      break;
    }

    case 0x04: {
      current_process_return(saved_state, getpid());
      break;
    }

    case 0x05: {
      current_process_return(saved_state, open((const char*) saved_state[0]));
      break;
    }

    case 0x06: {
      current_process_return(saved_state, close((int) saved_state[0]));
      break;
    }

    case 0x07: {
      char* buffer = (char*) saved_state[0];
      int count = (int) saved_state[1];
      int fd = (int) saved_state[2];
      current_process_return(saved_state, read(buffer, count, fd));
      break;
    }

    case 0x08: {
      const char* buffer = (const char*) saved_state[0];
      int count = (int) saved_state[1];
      int fd = (int) saved_state[2];
      current_process_return(saved_state, write(buffer, count, fd));
      break;
    }

    case 0x09: {
      const char* filename = (const char*) saved_state[0];
      int argc = (int) saved_state[1];
      const char** argv = (const char**) saved_state[2];
      current_process_return(saved_state, exec(filename, argc, argv));
      break;
    }

    default: {
      printf("Unknown System Call\n");
      while (true) {}
    }
  }

  printf("Should be a noreturn function\n");
  while (true) {}
  return 0;
}

