#include "machine.h"
#include "printf.h"
#include "process.h"

void user_mode()
{
  printf("Hello User Space\n");
}

Process::Process() : translation_table(VMM::TranslationTable::Granule::KB_4), context(VMM::kernel_to_phys_ptr((uint64_t) user_mode), 0x800000)
{
  // Basic Sanity Mapping
  for (uint64_t virtual_address = 0; virtual_address < 0x40000000; virtual_address += 0x1000)
  {
    translation_table.map_address(virtual_address, VMM::kernel_to_phys_ptr(virtual_address), VMM::TranslationTable::UnprivilegedAccess, VMM::TranslationTable::PageSize::KB_4);
  }
}

Process::~Process()
{
  
}

extern "C" void enter_process(struct ProcessContext*);

void Process::run()
{
  set_ELR_EL1(context.pc);
  set_SP_EL0(context.sp);
  set_SPSR_EL1(context.status_register);
  
  translation_table.set_ttbr0_el1();

  printf("Here\n");

  enter_process(&context);
}