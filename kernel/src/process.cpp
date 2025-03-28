#include "machine.h"
#include "printf.h"
#include "process.h"

void user_mode()
{
  printf("Hello User Space\n");
}

ProcessContext::ProcessContext() : translation_table(VMM::TranslationTable::Granule::KB_4)
{
  // Basic Sanity Mapping
  for (uint64_t virtual_address = 0; virtual_address < 0x40000000; virtual_address += 0x1000)
  {
    translation_table.map_address(virtual_address, VMM::kernel_to_phys_ptr(virtual_address), VMM::TranslationTable::UnprivilegedAccess, VMM::TranslationTable::PageSize::KB_4);
  }

  saved_state.pc = VMM::kernel_to_phys_ptr((uint64_t) user_mode);
  saved_state.sp = 0x800000;
}

ProcessContext::~ProcessContext()
{
  
}

void ProcessContext::enter_process()
{
  set_SPSR_EL1(0);
  set_ELR_EL1(saved_state.pc);
  set_SP_EL0(saved_state.sp);
    
  translation_table.set_ttbr0_el1();
  
  // uint8_t x = *((uint8_t*)0x0); 
  // x = x + 1;
  // *((uint8_t*)0x0) = 5;

  printf("%lu\n", get_TTBR0_EL1());

  exception_return();
}