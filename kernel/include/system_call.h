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
 * parameters being stored on the stack (see TECHNICALITIES Point 1). If a
 * system call has a return value, it will be placed in x0, with all other
 * registers remaining unchanged. If the system call has a void return type,
 * all registers remain unchanged.
 *
 * Negative return values signify that an error has occurred - see the
 * SystemCallErrorCode enum in the code below. Note that all system calls
 * share the same error code enum (even though not all errors are returnable
 * by all system calls). A return value of zero signals success with no
 * additional information, and a positive return value signals a successful
 * result returned (with the meaning of the result depending on the system
 * call). See below for specific system call details.
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

enum SystemCallErrorCode {
  NOT_IMPLEMENTED       = -2,
  INVALID_SYSTEM_CALL   = -3,
  INVALID_OPERATION     = -4,
  INVALID_POINTER       = -5,
  FILE_NOT_FOUND        = -6,
  INVALID_FD            = -7,
  INVALID_SEEK_TYPE     = -8,
  DATA_OVERFLOW         = -9
};

enum SeekType {
  ABSOLUTE = 2,
  RELATIVE = 3,
  END      = 4
};

extern uint64_t system_call_handler(uint16_t syscall_type, uint64_t* saved_state);

#endif
