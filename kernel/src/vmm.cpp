#include "physmem.h"

#include "machine.h"

namespace VMM
{
  uint64_t* get_stage_descriptor(uint64_t address, uint8_t level, uint64_t* stage_page)
  {
    uint32_t entry_number;

    switch(level)
    {
      case 0:
        entry_number = (address >> 39) & 0x1FF;
        break;
      case 1:
        entry_number = (address >> 30) & 0x1FF;
        break;
      case 2:
        entry_number = (address >> 21) & 0x1FF;
        break;
      case 3:
        entry_number = (address >> 12) & 0x1FF;
        break;
      default:
        // Panic : Unknown Level
        break;
    }

    return stage_page + entry_number;
  }

  void map_address(uint64_t virtual_address, uint64_t physical_address, uint64_t* base_table)
  {
    
  }

  void setup_virtual_memory_enviroment(void)
  {
    
  }
}

extern "C" void vmm_pageFault ()
{
  
}