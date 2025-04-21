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
      printf("TranslationTable: unsupported granule size %u\n", (unsigned)gran);
      while (true) {}
    }
    else if (gran == Granule::KB_64)
    {
      printf("TranslationTable: unsupported granule size %u\n", (unsigned)gran);
      while (true) {}
    }
    else 
    {
      printf("TranslationTable: unknown granule size %u\n", (unsigned)gran);
      while (true) {}
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
      return nullptr;
    }
    else if (granule_size == Granule::KB_64)
    {
      return nullptr;
    }
    else
    {
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
        printf("get_stage_descriptor: illegal level %u (va=0x%lx)\n", level, address);
        while (true) {}
    }
  }

  bool TranslationTable::map_address(uint64_t virtual_address, uint64_t physical_address, uint32_t flags, PageSize pg_sz)
  {
    if (pg_sz == PageSize::NONE || pg_sz == PageSize::KB_16 || pg_sz == PageSize::KB_64 || pg_sz == PageSize::MB_2 || pg_sz == PageSize::GB_1)
    {
      printf("map_address: unsupported PageSize %u\n", (unsigned)pg_sz);
      while (true) {}
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
        printf("map_address: L0 entry 0x%lx not a table (va=0x%lx)\n", *stage_0_descriptor, virtual_address);
        while (true) {}
      }

      uint64_t* stage_1_base_address = get_next_level(*stage_0_descriptor);
      uint64_t* stage_1_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 1, stage_1_base_address));

      if (!is_valid_descriptor(*stage_1_descriptor))
      {
        create_page_descriptor(stage_1_descriptor);
      }

      if (!is_page_descriptor(*stage_1_descriptor))
      {
        printf("map_address: L1 entry 0x%lx not a table (va=0x%lx)\n", *stage_1_descriptor, virtual_address);
        while (true) {}
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
        printf("map_address: L2 entry 0x%lx not a table (va=0x%lx)\n", *stage_2_descriptor, virtual_address);
        while (true) {}
      }

      uint64_t* stage_3_base_address = get_next_level(*stage_2_descriptor);
      uint64_t* stage_3_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 3, stage_3_base_address));

      uint64_t bit_mask = (1 << 10); // Set Access Flag

      if (flags & ExecuteNever)
        bit_mask |= (1UL << 54);
      
      if (flags & ReadOnlyPermission)
        bit_mask |= (1 << 7);

      if (flags & UnprivilegedAccess)
        bit_mask |= (1 << 6);

      if (flags & DeviceMemory)
      {
        *stage_3_descriptor = physical_address
          | bit_mask
          | (MAIR::get_mair_mask(MAIR::Attribute::Device_nGnRnE) << 2)
          | 0b10 /*page entry*/
          | 0b1 /*valid descriptor*/;
      }
      else
      {
        *stage_3_descriptor = physical_address
          | bit_mask
          | (MAIR::get_mair_mask(MAIR::Attribute::NormalMemory) << 2)
          | 0b10 /*page entry*/
          | 0b1 /*valid descriptor*/;
      }

      return true;
    }
    else if (granule_size == Granule::KB_16)
    {
      return false;
    }
    else if (granule_size == Granule::KB_64)
    {
      return false;
    }
    else
    {
      return false;
    }
  }

  bool TranslationTable::map_address(uint64_t virtual_address, uint32_t flags, PageSize pg_sz)
  {
    return map_address(virtual_address, reinterpret_cast<uint64_t>(PhysMem::allocate_frame()), flags, pg_sz);
  }

  bool TranslationTable::unmap_address(uint64_t virtual_address, PageSize pg_sz)
  {
    if (granule_size == Granule::KB_4)
    {
      if (pg_sz != PageSize::NONE && pg_sz != PageSize::KB_4 && pg_sz != PageSize::MB_2 && pg_sz != PageSize::GB_1)
      {
        printf("unmap_address: unsupported PageSize %u\n", (unsigned)pg_sz);
        while (true) {}
      }

      uint64_t* stage_0_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 0, base_address));

      if (!is_valid_descriptor(*stage_0_descriptor))
      {
        return true;
      }

      if (!is_page_descriptor(*stage_0_descriptor))
      {
        return false;
      }

      uint64_t* stage_1_base_address = get_next_level(*stage_0_descriptor);
      uint64_t* stage_1_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 1, stage_1_base_address));

      if (!is_valid_descriptor(*stage_1_descriptor))
      {
        return true;
      }

      if (!is_page_descriptor(*stage_1_descriptor))
      {
        if (pg_sz == PageSize::GB_1 || pg_sz == PageSize::NONE)
        {
          invalidate_entry(stage_1_descriptor);
          return true;
        }
        else
        {
          return false;
        }
      }

      uint64_t* stage_2_base_address = get_next_level(*stage_1_descriptor);
      uint64_t* stage_2_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 2, stage_2_base_address));

      if (!is_valid_descriptor(*stage_2_descriptor))
      {
        return true;
      }

      if (!is_page_descriptor(*stage_2_descriptor))
      {
        if (pg_sz == PageSize::MB_2 || pg_sz == PageSize::NONE)
        {
          invalidate_entry(stage_2_descriptor);
          return true;
        }
        else
        {
          return false;
        }
      }

      uint64_t* stage_3_base_address = get_next_level(*stage_2_descriptor);
      uint64_t* stage_3_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 3, stage_3_base_address));

      if (!is_valid_descriptor(*stage_3_descriptor))
      {
        return true;
      }

      if (is_page_descriptor(*stage_3_descriptor))
      {
        printf("unmap_address: L3 entry 0x%lx is page, expected table (va=0x%lx)\n", *stage_3_descriptor, virtual_address);
        while (true) {};
      }

      if (pg_sz == PageSize::KB_4 || pg_sz == PageSize::NONE)
      {
        invalidate_entry(stage_3_descriptor);
        return true;
      }

      return false;
    }
    else if (granule_size == Granule::KB_16)
    {
      return false;
    }
    else if (granule_size == Granule::KB_64)
    {
      return false;
    }
    else
    {
      return false;
    }
  }

  void TranslationTable::set_ttbr0_el1()
  {
    uint64_t prev_tcr_el1 = get_TCR_EL1();
    prev_tcr_el1 &= 0xFFFFFFFFFFFF0000;

    uint64_t tg0;

    if (granule_size == Granule::KB_4)
      tg0 = 0b00;
    else if (granule_size == Granule::KB_64)
      tg0 = 0b01;
    else if (granule_size == Granule::KB_16)
      tg0 = 0b10;
    else
    {
      printf("set_ttbr0_el1: unknown granule enum=%u\n", static_cast<unsigned>(granule_size));
      while (true) {}
    }

    uint64_t new_tcr_el1 = prev_tcr_el1
                            | (tg0 << 14) /*TG0*/
                            | 16; /*Settting T0 SZ*/

    set_TCR_EL1(new_tcr_el1);

    set_TTBR0_EL1(reinterpret_cast<uint64_t>(base_address));
    tlb_invalidate_all();
  }

  void TranslationTable::set_ttbr1_el1()
  {
    set_TTBR1_EL1(reinterpret_cast<uint64_t>(base_address));
    tlb_invalidate_all();
  }

  TranslationTable kernel_translation_table{TranslationTable::Granule::KB_4};

  void init()
  {
    // Allocate First 1 GB for kernel code/data
    for (uint64_t virtual_address = 0xFFFF000000000000; virtual_address < 0xFFFF000040000000; virtual_address += 0x1000)
    {
      kernel_translation_table.map_address(virtual_address, kernel_to_phys_ptr(virtual_address), 0, TranslationTable::PageSize::KB_4);
    }

    // Map the UART region (physical 0x3F201000 to virtual 0xFFFF00003F201000)
    uint64_t uart_phys_base = 0x3F201000;
    uint64_t uart_virt_base = 0xFFFF00003F201000;
    kernel_translation_table.map_address(uart_virt_base, uart_phys_base, TranslationTable::DeviceMemory, TranslationTable::PageSize::KB_4);

    // Map the GPIO region (physical 0x3F200000 to virtual 0xFFFF00003F200000)
    uint64_t gpio_phys_base = 0x3F200000;
    uint64_t gpio_virt_base = 0xFFFF00003F200000;
    kernel_translation_table.map_address(gpio_virt_base, gpio_phys_base, TranslationTable::DeviceMemory, TranslationTable::PageSize::KB_4);

    // Map the clock region (physical 0x3F101000 to virtual 0xFFFF00003F101000)
    // Note: Clock registers are part of the system timer/CM region on BCM2837
    uint64_t clock_phys_base = 0x3F101000;
    uint64_t clock_virt_base = 0xFFFF00003F101000;
    kernel_translation_table.map_address(clock_virt_base, clock_phys_base, TranslationTable::DeviceMemory, TranslationTable::PageSize::KB_4);

    // Map additional peripheral window (e.g., property mailbox at 0x3F00B880)
    for (uint64_t off = 0x0B880; off < 0x0C000; off += 0x1000)
    {
        uint64_t phys_addr = 0x3F000000 + off;
        uint64_t virt_addr = 0xFFFF00003F000000 + off;
        kernel_translation_table.map_address(virt_addr, phys_addr, TranslationTable::DeviceMemory, TranslationTable::PageSize::KB_4);
    }

    MAIR::setup_mair_el1();
    kernel_translation_table.set_ttbr1_el1();

    // Enable the MMU
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= 0x1; // Enable MMU
    asm volatile("msr sctlr_el1, %0" : : "r"(sctlr));
    asm volatile("isb");
  }

  void init_core()
  {
    MAIR::setup_mair_el1();
    kernel_translation_table.set_ttbr1_el1();

    // Ensure MMU is enabled for this core
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= 0x1;
    asm volatile("msr sctlr_el1, %0" : : "r"(sctlr));
    asm volatile("isb");
  }
}

extern "C" void vmm_pageFault()
{
}