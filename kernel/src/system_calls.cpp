#include "system_call.h"
#include "cores.h"
#include "event_loop.h"
#include "printf.h"
#include "process.h"
#include "ext2.h"

#define USER_MASK 0xFFFF'0000'0000'0000
#define IN_USER(ptr) ((((uint64_t) (ptr)) & USER_MASK) == USER_MASK)

// TODO:
// - Refactor calling and return convention for system calls
// - Move exit and yield implementations into separate methods?
// - Exit codes + fork + join + getpid

// [[noreturn]] void exit(int code) {}
// [[noreturn]] void yield() {}

Syscall::Result<int> fork() {
  return Syscall::NOT_IMPLEMENTED;
}

Syscall::Result<int> join(int pid) {
  return Syscall::NOT_IMPLEMENTED;
}

Syscall::Result<int> getpid() {
  return Syscall::NOT_IMPLEMENTED;
}

Syscall::Result<int> fopen(const char* filename) {
  Process* current_process = activeProcess[SMP::whichCore()];
  return current_process->file_open(filename);
}

Syscall::Result<int> close(int fd) {
  Process* current_process = activeProcess[SMP::whichCore()];
  return current_process->close_io_resource(fd);
}

IOResource* fetch_io_resource(int fd) {
  Process* current_process = activeProcess[SMP::whichCore()];
  return current_process->get_io_resource(fd);
}

Syscall::Result<long> read(char* buffer, long size, int fd) {
  if (!IN_USER(buffer)) return Syscall::INVALID_POINTER;
  IOResource* io_resource = fetch_io_resource(fd);
  if (io_resource == nullptr) return Syscall::INVALID_FD;
  return io_resource->read(buffer, size);
}

Syscall::Result<long> write(const char* buffer, long size, int fd) {
  if (!IN_USER(buffer)) return Syscall::INVALID_POINTER;
  IOResource* io_resource = fetch_io_resource(fd);
  if (io_resource == nullptr) return Syscall::INVALID_FD;
  return io_resource->write(buffer, size);
}

Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type, int fd) {
  IOResource* io_resource = fetch_io_resource(fd);
  if (io_resource == nullptr) return Syscall::INVALID_FD;
  return io_resource->seek(loc, seek_type);
}

Syscall::Result<int> exec(const char* filename, int argc, const char** argv) {
  if (!IN_USER(filename) || !IN_USER(argv)) return Syscall::INVALID_POINTER;
  Node* output = find_from_abs_path(filename);
  if (output == nullptr) return Syscall::FILE_NOT_FOUND;
  return Syscall::NOT_IMPLEMENTED;
}

template <typename T>
void process_return(uint64_t* saved_state, Syscall::Result<T> result) {
  result.set_state(saved_state);
  Process* current_process = activeProcess[SMP::whichCore()];
  current_process->save_state(saved_state);
  current_process->run();
}

uint64_t system_call_handler(uint16_t syscall_type, uint64_t* saved_state) {
  switch (syscall_type) {
    // 0x00: void exit(int code);
    case 0x00: {
      uint8_t current_core = SMP::whichCore();
      Process* current_process = activeProcess[current_core];
      delete current_process;
      activeProcess[current_core] = nullptr;
      event_loop();
      break;
    }

    // 0x01: void yield();
    case 0x01: {
      uint8_t current_core = SMP::whichCore();
      Process* current_process = activeProcess[current_core];
      current_process->save_state(saved_state);
      activeProcess[current_core] = nullptr;
      __asm__ volatile("dmb sy" ::: "memory");
      schedule_event([current_process] () { current_process->run(); });
      event_loop();
      break;
    }

    // 0x02: int fork();
    case 0x02: {
      Syscall::Result<int> result = fork();
      process_return(saved_state, result);
      break;
    }

    // 0x03: int join(int pid);
    case 0x03: {
      int pid = (int) saved_state[0];
      Syscall::Result<int> result = join(pid);
      process_return(saved_state, result);
      break;
    }

    // 0x04: int getpid();
    case 0x04: {
      Syscall::Result<int> result = getpid();
      process_return(saved_state, result);
      break;
    }

    // 0x05: int fopen(const char* filename);
    case 0x05: {
      const char* filename = (const char*) saved_state[0];
      Syscall::Result<int> result = fopen(filename);
      process_return(saved_state, result);
      break;
    }

    // 0x06: int close(int fd);
    case 0x06: {
      int fd = (int) saved_state[0];
      Syscall::Result<int> result = close(fd);
      process_return(saved_state, result);
      break;
    }

    // 0x07: long read(char* buffer, long size, int fd);
    case 0x07: {
      char* buffer = (char*) saved_state[0];
      long size = (long) saved_state[1];
      int fd = (int) saved_state[2];
      Syscall::Result<long> result = read(buffer, size, fd);
      process_return(saved_state, result);
      break;
    }

    // 0x08: long write(const char* buffer, long size, int fd);
    case 0x08: {
      const char* buffer = (const char*) saved_state[0];
      long size = (long) saved_state[1];
      int fd = (int) saved_state[2];
      Syscall::Result<long> result = write(buffer, size, fd);
      process_return(saved_state, result);
      break;
    }

    // 0x09: long seek(long loc, SeekType seek_type, int fd);
    case 0x09: {
      long loc = (long) saved_state[0];
      Syscall::SeekType seek_type = (Syscall::SeekType) saved_state[1];
      int fd = (int) saved_state[2];
      Syscall::Result<long> result = seek(loc, seek_type, fd);
      process_return(saved_state, result);
      break;
    }

    // 0x0A: int exec(const char* filename, int argc, const char** argv);
    case 0x0A: {
      const char* filename = (const char*) saved_state[0];
      int argc = (int) saved_state[1];
      const char** argv = (const char**) saved_state[2];
      Syscall::Result<int> result = exec(filename, argc, argv);
      process_return(saved_state, result);
      break;
    }

    default: {
      printf("Unknown System Call\n");
      Syscall::Result<int> result = Syscall::INVALID_SYSTEM_CALL;
      process_return(saved_state, result);
      break;
    }
  }

  printf("Kernel error: system_call_handler should not return\n");
  while (true) {}
  return 0;
}

#undef USER_MASK
#undef IN_USER

