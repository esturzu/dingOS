#include "physmem.h"
#include "machine.h"
#include "printf.h"
#include "vmm.h"
#include "definitions.h"

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
        return false;
      }

      uint64_t* stage_2_base_address = get_next_level(*stage_1_descriptor);
      uint64_t* stage_2_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 2, stage_2_base_address));

      if (!is_valid_descriptor(*stage_2_descriptor))
      {
        return true;
      }

      if (!is_page_descriptor(*stage_2_descriptor))
      {
        return false;
      }

      uint64_t* stage_3_base_address = get_next_level(*stage_2_descriptor);
      uint64_t* stage_3_descriptor = phys_to_kernel_ptr(get_stage_descriptor(virtual_address, 3, stage_3_base_address));

      if (!is_valid_descriptor(*stage_3_descriptor))
      {
        return true;
      }

      if (is_page_descriptor(*stage_3_descriptor))
      {
        return false;
      }

      invalidate_entry(stage_3_descriptor);

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

  void TranslationTable::set_ttbr0_el1()
  {
    asm volatile (
      "msr ttbr0_el1, %0\n"
      "isb\n"
      : : "r" (base_address)
    );
  }

  void TranslationTable::set_ttbr1_el1()
  {
    asm volatile (
      "msr ttbr1_el1, %0\n"
      "isb\n"
      : : "r" (base_address)
    );
  }

  static TranslationTable* kernel_table;

  void init()
  {
    // Set up MAIR_EL1 for memory attributes
    MAIR::setup_mair_el1();

    // Initialize the kernel page table
    kernel_table = new TranslationTable(TranslationTable::Granule::KB_4);

    // Map UART (0x3F201000 to 0xFFFF00003F201000)
    for (uint64_t va = UART0_BASE, pa = 0x3F201000; va < UART0_BASE + 0x1000; va += 0x1000, pa += 0x1000)
    {
      kernel_table->map_address(va, pa, TranslationTable::DeviceMemory | TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Map GPIO (0x3F200000 to 0xFFFF00003F200000)
    for (uint64_t va = GPIO_BASE, pa = 0x3F200000; va < GPIO_BASE + 0x1000; va += 0x1000, pa += 0x1000)
    {
      kernel_table->map_address(va, pa, TranslationTable::DeviceMemory | TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Map clock (0x3F101000 to 0xFFFF00003F101000)
    for (uint64_t va = 0xFFFF00003F101000, pa = 0x3F101000; va < 0xFFFF00003F101000 + 0x1000; va += 0x1000, pa += 0x1000)
    {
      kernel_table->map_address(va, pa, TranslationTable::DeviceMemory | TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Map mailbox (0x3F00B880 to 0xFFFF00003F00B880)
    for (uint64_t va = 0xFFFF00003F00B880, pa = 0x3F00B880; va < 0xFFFF00003F00B880 + 0x1000; va += 0x1000, pa += 0x1000)
    {
      kernel_table->map_address(va, pa, TranslationTable::DeviceMemory | TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Map LAN9118 Ethernet (0x3F300000 to 0xFFFF00003F300000)
    for (uint64_t va = LAN9118_BASE, pa = 0x3F300000; va < LAN9118_BASE + 0x1000; va += 0x1000, pa += 0x1000)
    {
      kernel_table->map_address(va, pa, TranslationTable::DeviceMemory | TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Map kernel virtual memory (identity mapping for now)
    for (uint64_t va = 0xFFFF000000000000; va < 0xFFFF000000100000; va += 0x1000)
    {
      kernel_table->map_address(va, va - 0xFFFF000000000000, TranslationTable::ExecuteNever, TranslationTable::PageSize::KB_4);
    }

    // Flush caches and TLB
    asm volatile (
      "dsb sy\n"
      "tlbi vmalle1\n"
      "dsb sy\n"
      "isb\n"
    );

    // Set TTBR0_EL1 to point to the kernel page table
    kernel_table->set_ttbr0_el1();
  }

  void init_core()
  {
    // Enable the MMU
    uint64_t sctlr;
    asm volatile (
      "mrs %0, sctlr_el1\n"
      "orr %0, %0, #1\n" // Enable MMU
      "msr sctlr_el1, %0\n"
      "isb\n"
      : "=r" (sctlr)
    );
  }
}