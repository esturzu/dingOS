/* PUBLIC SYSTEM CALL API
 *
 * OVERVIEW OF SYSTEM CALLS
 *
 * 0x00: void exit(int code);
 * 0x01: void yield();
 * 0x02: int fork();
 * 0x03: int join(int pid);
 * 0x04: int getpid();
 * 0x05: int fopen(const char* filename);
 * 0x06: int close(int fd);
 * 0x07: long read(char* buffer, long size, int fd);
 * 0x08: long write(const char* buffer, long size, int fd);
 * 0x09: long seek(long loc, SeekType seek_type, int fd);
 * 0x0A: int exec(const char* filename, int argc, const char** argv);
 *
 * CALLING CONVENTION
 *
 * From assembly, these system calls can be called with the "svc" instruction,
 * where the parameter is the system call ID - for example, "svc #1" would
 * execute a yield system call. System calls adhere to ARM 64-bit linkage
 * convention. All system calls so far only take integer types and/or memory
 * pointers as inputs, so the first eight parameters are stored in registers
 * x0 through x7 (in order, from left to right), with the remaining
 * parameters being stored on the stack (see TECHNICALITIES Point 1).
 *
 * RETURN CONVENTION
 *
 * If a system call has void return type, then all registers will remain
 * unchanged when it returns. Otherwise, the system call will edit registers
 * x0 and x1 and leave the remaining registers unchanged. In this convention,
 * x0 represents the return value, and x1 represents error metadata.
 *
 * The register x1 represents the status of the system call, given by the
 * ErrorCode enum below. If the call is successful, the value placed in x1 is
 * SUCCESS (0). If it is unsuccessful, a positive value will be placed in this
 * register representing the error that occurred. Note that these errors are
 * shared between all system calls (as in, all system calls follow the same
 * ErrorCode enum standard when it comes to what values represent what
 * errors). In general, x1 represents the "metadata" behind a system call.
 *
 * The register x0 represents the actual return value of the system call. What
 * this value represents depends on the individual system call; see the
 * details below. If x1 equals SUCCESS, x0 will contain the expected
 * result; if x1 does not equal SUCCESS, then x0 may optionally contain more
 * information about the error that occurred, depending on the system call.
 * 
 * INDIVIDUAL DESCRIPTIONS
 *
 * 0x00: Exits with a given code (passed through x0), where success is
 *       generally denoted through a code of 0. This system call will not
 *       return, and will kill the process and free its resources.
 *   
 * 0x01: Cooperatively yields and allows another process to run. When this
 *       process becomes active again, all register values will be set to
 *       exactly what they were just before the system call.
 *
 * 0x02: Conducts a fork system call. Currently unimplmemented; interface
 *       will come later.
 *
 * 0x03: Conducts a join system call. Currently unimplmemented; interface
 *       will come later.
 *
 * 0x04: Conducts a getpid system call. Currently unimplmemented; interface
 *       will come later.
 *
 * 0x05: Opens a file given an absolute path. If successful, it returns the
 *       file descriptor (fd) that should be used for all future system calls
 *       that interact with this file. Possible errors are FILE_NOT_FOUND and
 *       FD_OVERFLOW.
 *
 * 0x06: Closes an IO resource given its file descriptor (fd). If successful,
 *       it returns 0. Possible error is INVALID_FD (which would happen on a
 *       double close, for example). Note that, after closing, this file
 *       descriptor may be used by the kernel for other newly opened IO
 *       resources.
 *
 * 0x07: Reads from an IO resource given a buffer to read into, an amount to
 *       read (in bytes), and a file descriptor (fd). If successful, it
 *       returns the number of bytes read (which can be zero, if the amount of
 *       bytes to read is zero or if the position is at the end of the file).
 *       Possible errors are INVALID_POINTER, INVALID_FD, INVALID_OPERATION,
 *       INVALID_FILE_POS, and INVALID_SIZE.
 *
 * 0x08: Writes to an IO resource given a buffer to write from, an amount to
 *       write (in bytes), and a file descriptor (fd). If successful, it
 *       returns the number of bytes written (which can be zero, if the amount
 *       of bytes to write is zero). Possible errors are INVALID_POINTER,
 *       INVALID_FD, INVALID_OPERATION, and INVALID_SIZE. Currently
 *       unimplemented for file writes, and errors subject to change, though
 *       the calling convention should not.
 *
 * 0x09: Sets the current location for reads (and possibly writes, subject to
 *       change) to occur; it "seeks" a given position. Takes in a location, a
 *       seek type (given by the SeekType enum below), and a file descriptor
 *       (fd). The three seek types currently implemented are SEEK_ABSOLUTE
 *       (which moves to an absolute offset given by loc), SEEK_RELATIVE
 *       (which moves to an offset relative to the current position), and
 *       SEEK_ENDING (which moves to an offset relative to the end of the
 *       file). Note that this system call can take the position before or
 *       after file boundaries without error (although it will error when you
 *       read and write to these locations). If successful, it returns the
 *       new offset. Possible errors are INVALID_FD, INVALID_OPERATION, and
 *       INVALID_SEEK_TYPE.
 *
 * 0x0A: Executes a given ELF file, currently to be implemented.
 *
 * TODO finish these descriptions
 *
 * TECHNICALITIES
 *
 * 1. By ARM calling convention, arguments after the first eight are placed on
 *    the stack in reverse order (from right to left). Note that this means
 *    earlier arguments will be placed at lower memory addresses than later
 *    arguments, since the stack grows towards lower memory addresses. For
 *    example, the ninth argument from the left will be pushed later, and will
 *    occupy a lower memory address, than the tenth argument.
 */

#include "stdint.h"

#ifndef SYS_CALL_H
#define SYS_CALL_H

/* Namespace for system call convertions */
namespace Converter {
  template <typename T> uint64_t bits_to_u64(T a);

  template <> inline uint64_t bits_to_u64<int>(int a) {
    return (unsigned int) a;
  }

  template <> inline uint64_t bits_to_u64<long>(long a) {
    return (unsigned long) a;
  }
}

/* Namespace for system call return information */
namespace Syscall {
  /* Error codes for system calls (for setting the x1 register) */
  enum ErrorCode : long {
    SUCCESS             = 0,
    NOT_IMPLEMENTED     = 1,
    INVALID_SYSTEM_CALL = 2,
    INVALID_FD          = 3,
    INVALID_OPERATION   = 4,
    INVALID_SIZE        = 5,
    INVALID_FILE_POS    = 6,
    INVALID_SEEK_TYPE   = 7,
    INVALID_POINTER     = 8,
    FILE_NOT_FOUND      = 9,
    FD_OVERFLOW         = 10,
    DATA_OVERFLOW       = 11
  };

  /* Seek types (for the seek system call) */
  enum SeekType {
    SEEK_ABSOLUTE = 1,
    SEEK_RELATIVE = 2,
    SEEK_ENDING   = 3
  };

  /* Stores system call results (for setting the x0 and x1 registers) */
  template <typename T>
  struct Result {
    T data;
    ErrorCode code;

    Result(T data, ErrorCode code) : data(data), code(code) {}
    Result(ErrorCode code) : data(0), code(code) {}
    Result(T data) : data(data), code(SUCCESS) {}

    void set_state(uint64_t* saved_state) {
      saved_state[0] = Converter::bits_to_u64<T>(data);
      saved_state[1] = code;
    }
  };
}

/* System call handler */
extern uint64_t system_call_handler(uint16_t syscall_type, uint64_t* saved_state);

#endif
