#include "stdint.h"

extern "C" uint64_t get_CurrentEL();

extern "C" uint64_t get_TTBR0_EL1();
extern "C" uint64_t get_TTBR1_EL1();
extern "C" uint64_t get_MAIR_EL1();
extern "C" uint64_t get_SCTLR_EL1();
extern "C" uint64_t get_TCR_EL1();
extern "C" uint64_t get_ESR_EL1();
extern "C" uint64_t get_FAR_EL1();

extern "C" uint64_t get_CNTP_CTL_EL0();
extern "C" uint64_t get_CNTP_CVAL_EL0();
extern "C" uint64_t get_CNTP_TVAL_EL0();

extern "C" void set_TTBR0_EL1(uint64_t val);
extern "C" void set_TTBR1_EL1(uint64_t val);
extern "C" void set_MAIR_EL1(uint64_t val);
extern "C" void set_SCTLR_EL1(uint64_t val);
extern "C" void set_TCR_EL1(uint64_t val);
extern "C" void set_VBAR_EL1(void* val);

extern "C" void tlb_invalidate_all();

extern "C" void set_SPSR_EL1(uint64_t val);
extern "C" void set_ELR_EL1(uint64_t val);
extern "C" void set_SP_EL0(uint64_t val);

extern "C" void set_CNTKCTL_EL1(uint64_t val);
extern "C" void set_CNTP_CTL_EL0(uint64_t val);
extern "C" void set_CNTP_CVAL_EL0(uint64_t val);
extern "C" void set_CNTP_TVAL_EL0(uint64_t val);

extern "C" void exception_return();

extern "C" void* el1_vector_table;