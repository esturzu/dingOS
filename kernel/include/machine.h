#include "stdint.h"

extern "C" uint64_t get_CurrentEL();
extern "C" void set_VBAR_EL1(void* vector_table);

extern "C" void* el1_vector_table;