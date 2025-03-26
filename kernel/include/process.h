#include "vmm.h"

#ifndef PROCESS_H
#define PROCESS_H

class ProcessContext
{

  VMM::TranslationTable translation_table;

  struct
  {
    uint64_t pc;
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    uint64_t x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30;
    uint64_t sp;
  } saved_state;

public:

  ProcessContext();
  ~ProcessContext();

  void enter_process ();

};

#endif