#include "vmm.h"
#include "ioresource.h"

#ifndef PROCESS_H
#define PROCESS_H

struct ProcessContext
{
  uint64_t pc;
  uint64_t sp;
  uint64_t status_register = {0x0};
  uint64_t x0 {0};
  uint64_t x1 {0};
  uint64_t x2 {0};
  uint64_t x3 {0};
  uint64_t x4 {0};
  uint64_t x5 {0};
  uint64_t x6 {0};
  uint64_t x7 {0};
  uint64_t x8 {0};
  uint64_t x9 {0};
  uint64_t x10 {0};
  uint64_t x11 {0};
  uint64_t x12 {0};
  uint64_t x13 {0};
  uint64_t x14 {0};
  uint64_t x15 {0};
  uint64_t x16 {0};
  uint64_t x17 {0};
  uint64_t x18 {0};
  uint64_t x19 {0};
  uint64_t x20 {0};
  uint64_t x21 {0};
  uint64_t x22 {0};
  uint64_t x23 {0};
  uint64_t x24 {0};
  uint64_t x25 {0};
  uint64_t x26 {0};
  uint64_t x27 {0};
  uint64_t x28 {0};
  uint64_t x29 {0};
  uint64_t x30 {0};
  uint64_t x31 {0}; // Link Register

  ProcessContext() : pc(0), sp(0) {}
  ProcessContext(uint64_t entry_point, uint64_t initial_sp) : pc(entry_point), sp(initial_sp) {}
};

class Process
{
  static constexpr int NUM_IO_RESOURCES = 16;
  ProcessContext context;
  VMM::TranslationTable translation_table;
  IOResource* resources[NUM_IO_RESOURCES];

  int find_unused_fd();

public:

  Process();
  ~Process();

  void run();
  void save_state(uint64_t* register_frame);
  void map_range(uint64_t start, uint64_t end);
  void set_entry_point(uint64_t entry);
  IOResource* get_io_resource(int fd);
  int file_open(const char* name);
  int close_io_resource(int fd);
};

extern Process* activeProcess[4];

#endif
