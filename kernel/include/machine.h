#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t get_CurrentEL();

uint64_t get_TTBR0_EL1();
uint64_t get_TTBR1_EL1();
uint64_t get_MAIR_EL1();
uint64_t get_SCTLR_EL1();
uint64_t get_TCR_EL1();
uint64_t get_ESR_EL1();
uint64_t get_FAR_EL1();

uint64_t get_CNTP_CTL_EL0();
uint64_t get_CNTP_CVAL_EL0();
uint64_t get_CNTP_TVAL_EL0();

uint64_t get_SPSR_EL1();
uint64_t get_ELR_EL1();
uint64_t get_SP_EL0();

void set_DAIFClr_all();  // clears (un‑masks) A,I,F
void set_DAIFSet_all();  // sets   (masks)  A,I,F

void set_TTBR0_EL1(uint64_t val);
void set_TTBR1_EL1(uint64_t val);
void set_MAIR_EL1(uint64_t val);
void set_SCTLR_EL1(uint64_t val);
void set_TCR_EL1(uint64_t val);
void set_VBAR_EL1(void* val);

void tlb_invalidate_all();

void set_SPSR_EL1(uint64_t val);
void set_ELR_EL1(uint64_t val);
void set_SP_EL0(uint64_t val);

void set_CNTKCTL_EL1(uint64_t val);
void set_CNTP_CTL_EL0(uint64_t val);
void set_CNTP_CVAL_EL0(uint64_t val);
void set_CNTP_TVAL_EL0(uint64_t val);

void set_stack_pointer(uint8_t* val);

void exception_return();

extern void* el1_vector_table;

#ifdef __cplusplus
}
#endif

uint32_t ticks_ms();      // milliseconds since boot
void delay_ms(uint32_t);  // busy-wait
