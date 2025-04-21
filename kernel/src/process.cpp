#include "machine.h"
#include "printf.h"
#include "process.h"
#include "cores.h"

Process* activeProcess[4] = {nullptr, nullptr, nullptr, nullptr};


void user_mode()
{
  while(1)
  {
    asm volatile (
      "mov x0, #0"
    );
  }
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

  activeProcess[SMP::whichCore()] = this;

  debug_printf("Entering Process\n");

  enter_process(&context);
}

void Process::save_state(uint64_t* register_frame)
{
  context.pc = get_ELR_EL1();
  context.sp = get_SP_EL0();
  context.status_register = get_SPSR_EL1();
  context.x0 = register_frame[0];
  context.x1 = register_frame[0];
  context.x2 = register_frame[0];
  context.x3 = register_frame[0];
  context.x4 = register_frame[0];
  context.x5 = register_frame[0];
  context.x6 = register_frame[0];
  context.x7 = register_frame[0];
  context.x8 = register_frame[0];
  context.x9 = register_frame[0];
  context.x10 = register_frame[0];
  context.x11 = register_frame[0];
  context.x12 = register_frame[0];
  context.x13 = register_frame[0];
  context.x14 = register_frame[0];
  context.x15 = register_frame[0];
  context.x16 = register_frame[0];
  context.x17 = register_frame[0];
  context.x18 = register_frame[0];
  context.x19 = register_frame[0];
  context.x20 = register_frame[0];
  context.x21 = register_frame[0];
  context.x22 = register_frame[0];
  context.x23 = register_frame[0];
  context.x24 = register_frame[0];
  context.x25 = register_frame[0];
  context.x26 = register_frame[0];
  context.x27 = register_frame[0];
  context.x28 = register_frame[0];
  context.x29 = register_frame[0];
  context.x30 = register_frame[0];
  context.x31 = register_frame[0];
}

void Process::map_range(uint64_t start, uint64_t end) {
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto pageSize = VMM::TranslationTable::PageSize::KB_4;
  for (uint64_t a = start & (-4096); a < end; a += 4096) {
    translation_table.map_address(a, flags, pageSize);
  }
}

void Process::set_entry_point(uint64_t entry) {
  context.pc = entry;
}

