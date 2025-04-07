#include "stdint.h"

#ifndef SYS_CALL_H
#define SYS_CALL_H

extern uint64_t system_call_handler (uint16_t syscall_type, uint64_t* saved_state);

#endif