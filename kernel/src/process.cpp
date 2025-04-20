#include "machine.h"
#include "printf.h"
#include "process.h"
#include "cores.h"
#include "system_call.h"

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

  // User Space Stack Mapping
  context.sp = 0;
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto page_size = VMM::TranslationTable::PageSize::KB_4;
  for (uint64_t a = 0xFFFF'FFFF'FF00'0000; a != 0; a += 0x1000) {
    translation_table.map_address(a, flags, page_size);
  }

  // Fills IO Resources
  static_assert(NUM_IO_RESOURCES >= 3, "Kernel error: Num IO < 3");
  resources[0] = new StandardInput();
  resources[1] = new StandardOutput();
  resources[2] = new StandardError();
  for (int i = 3; i < NUM_IO_RESOURCES; i++) {
    resources[i] = nullptr;
  }
}

Process::~Process()
{
  // TODO free memory mappings
  // TODO free filesystem attributes
  // Deletes IO Resources
  for (int i = 0; i < NUM_IO_RESOURCES; i++) {
    if (resources[i] != nullptr) delete resources[i];
  }
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
  context.x1 = register_frame[1];
  context.x2 = register_frame[2];
  context.x3 = register_frame[3];
  context.x4 = register_frame[4];
  context.x5 = register_frame[5];
  context.x6 = register_frame[6];
  context.x7 = register_frame[7];
  context.x8 = register_frame[8];
  context.x9 = register_frame[9];
  context.x10 = register_frame[10];
  context.x11 = register_frame[11];
  context.x12 = register_frame[12];
  context.x13 = register_frame[13];
  context.x14 = register_frame[14];
  context.x15 = register_frame[15];
  context.x16 = register_frame[16];
  context.x17 = register_frame[17];
  context.x18 = register_frame[18];
  context.x19 = register_frame[19];
  context.x20 = register_frame[20];
  context.x21 = register_frame[21];
  context.x22 = register_frame[22];
  context.x23 = register_frame[23];
  context.x24 = register_frame[24];
  context.x25 = register_frame[25];
  context.x26 = register_frame[26];
  context.x27 = register_frame[27];
  context.x28 = register_frame[28];
  context.x29 = register_frame[29];
  context.x30 = register_frame[30];
  context.x31 = register_frame[31];
}

void Process::map_range(uint64_t start, uint64_t end) {
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto page_size = VMM::TranslationTable::PageSize::KB_4;
  for (uint64_t a = start & (-4096); a < end; a += 4096) {
    translation_table.map_address(a, flags, page_size);
  }
}

void Process::set_entry_point(uint64_t entry) {
  context.pc = entry;
}

IOResource* Process::get_io_resource(int fd) {
  bool out_of_bounds = ((unsigned int) fd) >= NUM_IO_RESOURCES;
  return out_of_bounds ? nullptr : resources[fd];
}

int Process::find_unused_fd() {
  for (int i = 0; i < NUM_IO_RESOURCES; i++) {
    if (resources[i] == nullptr) return i;
  }
  return SystemCallErrorCode::DATA_OVERFLOW;
}

int Process::file_open(const char* filename) {
  int fd = find_unused_fd();
  if (fd < 0) return fd;
  FileResource* resource = new FileResource();
  int result = resource->open(filename);
  if (result < 0) {
    delete resource;
    return result;
  }
  resources[fd] = resource;
  return fd;
}

int Process::close_io_resource(int fd) {
  if (resources[fd] == nullptr) return SystemCallErrorCode::INVALID_FD;
  delete resources[fd];
  resources[fd] = nullptr;
  return 0;
}

