#include "physmem.h"

#include "machine.h"
#include "printf.h"
#include "vmm.h"

namespace VMM
{
  TranslationTable::TranslationTable(enum Granule gran) : granule_size(gran)
  {
    base_address = (uint64_t*) PhysMem::allocate_frame();

    if (gran == Granule::KB_4)
    {
      uint64_t* kernel_base_address = phys_to_kernel_ptr(base_address);

      for (uint32_t entry = 0; entry < 512 /*4096 bytes / 8 bytes*/; entry++)
      {
        kernel_base_address[entry] = 0;
      }
    }
    else if (gran == Granule::KB_16)
    {
      // PANIC: Unsupported Granule Size
    }
    else if (gran == Granule::KB_64)
    {
      // PANIC: Unsupported Granule Size
    }
    else 
    {
      // PANIC: Unknown Granule Size
    }
  }

  TranslationTable::TranslationTable(enum Granule gran, uint64_t* base_addr) : granule_size(gran), base_address(base_addr)
  {

  }

  TranslationTable::~TranslationTable()
  {
    // TODO: Free Entries
  }

  inline bool TranslationTable::is_valid_descriptor(uint64_t entry)
  {
    return entry & 0b1;
  }

  inline bool TranslationTable::is_page_descriptor(uint64_t entry)
  {
    return entry & 0b10;
  }

  inline uint64_t* TranslationTable::get_next_level(uint64_t entry)
  {
    if (granule_size == Granule::KB_4)
    {
      return reinterpret_cast<uint64_t*>(entry & 0x0000FFFFFFFFF000);
    }
    else if (granule_size == Granule::KB_16)
    {
      // Unsupported Granule Size
      return nullptr;
    }
    else if (granule_size == Granule::KB_64)
    {
      // Unsupported Granule Size
      return nullptr;
    }
    else
    {
      // Unsupported Granule Size
      return nullptr;
    }
  }

  inline void TranslationTable::invalidate_entry(uint64_t* entry)
  {
    *entry = 0;
  }

  inline void TranslationTable::create_page_descriptor(uint64_t* entry)
  {
    uint64_t next_level_address = reinterpret_cast<uint64_t>(PhysMem::allocate_frame());
    *entry = next_level_address 
              | 0b10 /*page descriptor flag*/ 
              | 0b1 /*valid descriptor flag*/;
  }

  uint64_t* TranslationTable::get_stage_descriptor(uint64_t address, uint8_t level, uint64_t* stage_page)
  {
    if (granule_size != Granule::KB_4)
      return nullptr;

    switch(level)
    {
      case 0:
        {
          uint32_t entry_number = (address >> 39) & 0x1FF;
          return stage_page + entry_number;
        }
      case 1:
        {
          uint32_t entry_number = (address >> 30) & 0x1FF;
          return stage_page + entry_number;
        }
      case 2:
        {
          uint32_t entry_number = (address >> 21) & 0x1FF;
          return stage_page + entry_number;
        }
      case 3:
        {
          uint32_t entry_number = (address >> 12) & 0x1FF;
          return stage_page + entry_number;
        }
      default:
        // Panic : Unknown Level
        return nullptr;
    }
  }

  bool TranslationTable::map_address(uint64_t virtual_address, uint64_t physical_address, PageSize pg_sz)
  {
    if (pg_sz == PageSize::NONE || pg_sz == PageSize::KB_16 || pg_sz == PageSize::KB_64 || pg_sz == PageSize::MB_2 || pg_sz == PageSize::GB_1)
    {
      // Unsupported Page Sizes
      return false;
    }

    if (granule_size == Granule::KB_4)
    {
      uint64_t* stage_0_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 0, base_address));
      
      if (!is_valid_descriptor(*stage_0_descriptor))
      {
        create_page_descriptor(stage_0_descriptor);
      }

      if (!is_page_descriptor(*stage_0_descriptor))
      {
        // PANIC: Invalid Level for Block Descriptor
        return false;
      }

      uint64_t* stage_1_base_address = get_next_level(*stage_0_descriptor);
      uint64_t* stage_1_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 1, stage_1_base_address));

      if (!is_valid_descriptor(*stage_1_descriptor))
      {
        create_page_descriptor(stage_1_descriptor);
      }

      if (!is_page_descriptor(*stage_1_descriptor))
      {
        // PANIC: Invalid Level for Block Descriptor
        return false;
      }

      uint64_t* stage_2_base_address = get_next_level(*stage_1_descriptor);
      uint64_t* stage_2_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 2, stage_2_base_address));

      if (!is_valid_descriptor(*stage_2_descriptor))
      {
        create_page_descriptor(stage_2_descriptor);
        *stage_2_descriptor = *stage_2_descriptor; 
      }

      if (!is_page_descriptor(*stage_2_descriptor))
      {
        // PANIC: Invalid Level for Block Descriptor
        return false;
      }

      uint64_t* stage_3_base_address = get_next_level(*stage_2_descriptor);
      uint64_t* stage_3_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 3, stage_3_base_address));

      *stage_3_descriptor = physical_address
                            | (1 << 10) /*access flag*/
                            | (MAIR::get_mair_mask(MAIR::Attribute::NormalMemory) << 2)
                            | 0b10 /*page entry*/
                            | 0b1 /*valid descriptor*/;
      return true;
    }
    else if (granule_size == Granule::KB_16)
    {
      // Unsupported Granule Size
      return false;
    }
    else if (granule_size == Granule::KB_64)
    {
      // Unsupported Granule Size
      return false;
    }
    else
    {
      // Unsupported Granule Size
      return false;
    }
  }

  bool TranslationTable::map_address(uint64_t virtual_address, PageSize pg_sz)
  {
    return map_address(virtual_address, reinterpret_cast<uint64_t>(PhysMem::allocate_frame()), pg_sz);
  }


  bool TranslationTable::unmap_address(uint64_t virtual_address, PageSize pg_sz)
  {
    if (granule_size == Granule::KB_4)
    {
      if (pg_sz != PageSize::NONE || pg_sz != PageSize::KB_4 || pg_sz != PageSize::MB_2 || pg_sz != PageSize::GB_1)
      {
        // Invalid Page Size
        return false;
      }

      uint64_t* stage_0_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 0, base_address));
      
      if (!is_valid_descriptor(*stage_0_descriptor))
      {
        // Not a valid descriptor to this virtual address
        return true;
      }

      if (!is_page_descriptor(*stage_0_descriptor))
      {
        return false;
      }

      uint64_t* stage_1_base_address = phys_to_kernel_ptr(get_next_level(*stage_0_descriptor));
      uint64_t* stage_1_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 1, stage_1_base_address));

      if (!is_valid_descriptor(*stage_1_descriptor))
      {
        // Not a valid descriptor to this virtual address
        return true;
      }

      if (!is_page_descriptor(*stage_1_descriptor))
      {
        if (pg_sz == PageSize::GB_1 || pg_sz == PageSize::NONE)
        {
          // TODO: free frame
          invalidate_entry(stage_1_descriptor);
          return true;
        }
        else
        {
          // Incorrect Page Size Being Removed
          return false;
        }
      }

      uint64_t* stage_2_base_address = phys_to_kernel_ptr(get_next_level(*stage_1_descriptor));
      uint64_t* stage_2_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 2, stage_2_base_address));

      if (!is_valid_descriptor(*stage_2_descriptor))
      {
        // Not a valid descriptor to this virtual address
        return true;
      }

      if (!is_page_descriptor(*stage_2_descriptor))
      {
        if (pg_sz == PageSize::MB_2 || pg_sz == PageSize::NONE)
        {
          // TODO: free frame
          invalidate_entry(stage_2_descriptor);
          return true;
        }
        else
        {
          // Incorrect Page Size Being Removed
          return false;
        }
      }

      uint64_t* stage_3_base_address = phys_to_kernel_ptr(get_next_level(*stage_2_descriptor));
      uint64_t* stage_3_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 3, stage_3_base_address));

      if (!is_valid_descriptor(*stage_3_descriptor))
      {
        // Not a valid descriptor to this virtual address
        return true;
      }

      if (is_page_descriptor(*stage_3_descriptor))
      {
        // PANIC: Can't have a page descriptor at the third level
        return false;
      }

      if (pg_sz == PageSize::KB_4 || pg_sz == PageSize::NONE)
      {
        // TODO: free frame
        invalidate_entry(stage_3_descriptor);
        return true;
      }

      // Incorrect Page Size Being Removed
      return false;
    }
    else if (granule_size == Granule::KB_16)
    {
      // Unsupported Granule Size
      return false;
    }
    else if (granule_size == Granule::KB_64)
    {
      // Unsupported Granule Size
      return false;
    }
    else
    {
      // Unsupported Granule Size
      return false;
    }
  }

  void TranslationTable::set_ttbr1_el1()
  {
    set_TTBR1_EL1(reinterpret_cast<uint64_t>(base_address));
    tlb_invalidate_all();
  }

  TranslationTable kernel_translation_table{TranslationTable::Granule::KB_4};

  void init()
  {
    // Allocate First 1 GB
    for (uint64_t virtual_address = 0xFFFF000000000000; virtual_address < 0xFFFF000040000000; virtual_address += 0x1000)
    {
      kernel_translation_table.map_address(virtual_address, kernel_to_phys_ptr(virtual_address), TranslationTable::PageSize::KB_4);
    }

    MAIR::setup_mair_el1();

    kernel_translation_table.set_ttbr1_el1();
  }

  void init_core()
  {

    MAIR::setup_mair_el1();

    kernel_translation_table.set_ttbr1_el1();
  }
}

extern "C" void vmm_pageFault ()
{
  
}