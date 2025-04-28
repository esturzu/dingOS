#include "machine.h"
#include "printf.h"
#include "process.h"
#include "cores.h"
#include "physmem.h"
#include "system_call.h"
#include "physmem.h"

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

Process::Process() : translation_table(VMM::TranslationTable::Granule::KB_4), context(VMM::kernel_to_phys_ptr((uint64_t) user_mode), STACK_HIGH_EXCLUSIVE)
{
  // Basic Sanity Mapping
  for (uint64_t virtual_address = 0; virtual_address < 0x40000000; virtual_address += 0x1000)
  {
    translation_table.map_address(virtual_address, VMM::kernel_to_phys_ptr(virtual_address), VMM::TranslationTable::UnprivilegedAccess, VMM::TranslationTable::PageSize::KB_4);
  }

  // User Space Stack Mapping
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto page_size = VMM::TranslationTable::PageSize::KB_4;
  map_range(STACK_LOW_INCLUSIVE, STACK_HIGH_EXCLUSIVE);

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

  // Make TTBR0 point at the process page‑table
  translation_table.set_ttbr0_el1();

  // Mark this process as “currently running” on this core
  activeProcess[SMP::whichCore()] = this;

  // Hand control to an assembler stub that loads ELR/SPSR/SP_EL0
  // from context and executes `eret` into EL0
  debug_printf("Entering Process\n");
  enter_process(&context);

  uint64_t el, spsr;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    asm volatile("mrs %0, SPSR_EL1"  : "=r"(spsr));
    debug_printf("[run] still in EL%llu, SPSR_EL1=%016llx\n",
                 (el >> 2) & 3, (unsigned long long)spsr);
    /* ------------------------------------------------------ */

    asm volatile(
        "msr    elr_el1,  x0 \n"
        "msr    sp_el0,   x1 \n"
        "msr    spsr_el1, x2 \n"
        "eret                 ");

  // Never returns
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

// Maps the memory addresses in the range from start (inclusive) to end
// (exclusive), where it pads with extra bytes if needed to page align
void Process::map_range(uint64_t start, uint64_t end) {
  if (((start | end) & 0xFFF) != 0) {
    printf("WARNING: START OR END MISALIGNED\n");
  }
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto page_size = VMM::TranslationTable::PageSize::KB_4;
  start &= (uint64_t) -0x1000;
  end = (end + 0xFFF) & (uint64_t) -0x1000;
  for (uint64_t a = start; a != end; a += 0x1000) {
    translation_table.map_address(a, flags, page_size);
  }
}

void Process::vm_load(uint64_t vaddr, uint64_t filesz, uint64_t memsz,
                      const char* data) {
  if ((vaddr & 0xFFF) != 0) {
    printf("WARNING: VM LOAD START MISALIGNED\n");
    return;
  }

  uint64_t num_pages = (memsz + 0xFFF) >> 12;
  char** pages = new char*[num_pages];
  constexpr uint64_t flags = VMM::TranslationTable::UnprivilegedAccess;
  constexpr auto page_size = VMM::TranslationTable::PageSize::KB_4;
  for (uint64_t i = 0, a = vaddr; i < num_pages; i++, a += 0x1000) {
    void* frame = PhysMem::allocate_frame();
    pages[i] = (char*) VMM::phys_to_kernel_ptr(frame);
    translation_table.map_address(a, (uint64_t) frame, flags, page_size);
  }

  uint64_t i = 0;
  for (; i < filesz; i++) pages[i >> 12][i & 0xFFF] = data[i];
  for (; i < memsz; i++) pages[i >> 12][i & 0xFFF] = 0;
  delete[] pages;
}

// Sets the entry point of a program
void Process::set_entry_point(uint64_t entry) {
  context.pc = entry;
}

// Returns the IO resource with the given file descriptor, or nullptr if the
// file descriptor does not point to a valid open resource
IOResource* Process::get_io_resource(int fd) {
  bool out_of_bounds = ((unsigned int) fd) >= NUM_IO_RESOURCES;
  return out_of_bounds ? nullptr : resources[fd];
}

// Finds an unused and available file descriptor, or returns -1 if none are
// available
int Process::find_unused_fd() {
  for (int i = 0; i < NUM_IO_RESOURCES; i++) {
    if (resources[i] == nullptr) return i;
  }
  return -1;
}

// Opens a file by creating a new file resource, and returns the file
// descriptor or -1 if an error occurred
Syscall::Result<int> Process::file_open(const char* filename) {
  int fd = find_unused_fd();
  if (fd == -1) return Syscall::FD_OVERFLOW;
  FileResource* resource = new FileResource();
  Syscall::ErrorCode code = resource->open(filename);
  if (code == Syscall::SUCCESS) {
    resources[fd] = resource;
    return fd;
  } else {
    delete resource;
    return code;
  }
}

// Closes a file and destroys its resources, or returns INVALID_FD
// if the file descriptor is invalid
Syscall::Result<int> Process::close_io_resource(int fd) {
  IOResource* resource = get_io_resource(fd);
  if (resource == nullptr) return Syscall::INVALID_FD;
  delete resource;
  resources[fd] = nullptr;
  return Syscall::SUCCESS;
}

