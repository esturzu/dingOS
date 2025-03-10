// Citations
// Armv8 A-profile Architecture Reference Manual

#include "stdint.h"
#include "machine.h"

#ifndef VMM_H
#define VMM_H

namespace VMM
{
  struct MAIR
  {
    enum Attribute : uint8_t
    {
      NormalMemory = 0,
      Device_nGnRnE = 1
    };

    static void setup_mair_el1()
    {
      int memory_attributes[8] = { 0b01110111 /*Normal memory, Outer Write-Back Transient, Outer Read-Allocate, Outer Write-Allocate, Inner Write-Back Transient, Inner Read-Allocate, Inner Write-Allocate*/, 
                                    0b00001100 /*Device-nGnRnE memory*/, 
                                    0b00000000 /*Unused*/, 
                                    0b00000000 /*Unused*/, 
                                    0b00000000 /*Unused*/, 
                                    0b00000000 /*Unused*/, 
                                    0b00000000 /*Unused*/, 
                                    0b00000000 /*Unused*/};

      uint64_t mair_value = 0;
      for (uint8_t attr = 0; attr < 8 /*number of attributes*/; attr++)
      {
        mair_value |= memory_attributes[attr] << (attr * 8);
      }
      set_MAIR_EL1(mair_value);
    }

    inline static uint8_t get_mair_mask(Attribute attr)
    {
      return static_cast<uint8_t>(attr);
    }
  };

  class TranslationTable
  {

  public:

    enum class Granule : uint8_t
    {
      KB_4,
      KB_16,
      KB_64
    };

    enum class PageSize : uint8_t
    {
      NONE = 0,
      KB_4,
      KB_16,
      KB_64,
      MB_2,
      GB_1
    };

  private:

    enum Granule granule_size;
    uint64_t* base_address;

    enum APTable
    {
      NoEffect = 0b00,
      PrivRW = 0b01,
      PrivR_UnprivR = 0b10,
      PrivR = 0b11
    };

    inline bool is_valid_descriptor(uint64_t entry);
    inline bool is_page_descriptor(uint64_t entry);
    inline uint64_t* get_next_level(uint64_t entry);

    inline void invalidate_entry(uint64_t* entry);
    inline void create_page_descriptor(uint64_t* entry);

    uint64_t* get_stage_descriptor(uint64_t address, uint8_t level, uint64_t* stage_page);

  public:

    TranslationTable(enum Granule gran);
    TranslationTable(enum Granule gran, uint64_t* base_addr);
    ~TranslationTable();

    bool unmap_address(uint64_t virtual_address, PageSize pg_sz = PageSize::NONE);
    bool map_address(uint64_t virtual_address, uint64_t physical_address, PageSize pg_sz = PageSize::NONE);
    bool map_address(uint64_t virtual_address, PageSize pg_sz = PageSize::NONE);

    void set_ttbr1_el1();
  };

  extern void init();

  /**
   * @brief Changes a pointer to the kernel into a physical address pointer
   *
   * @param ptr  pointer to kernel space
   * @return T   pointer to virtual space
   */
  template <typename T>
  inline T kernel_to_phys_ptr(T ptr)
  {
    return reinterpret_cast<T>(reinterpret_cast<uint64_t>(ptr) ^ 0xFFFF000000000000);
  }

  /**
   * @brief Changes a pointer to the physical address into a kernel pointer
   *
   * @param ptr  pointer to virtual space
   * @return T   pointer to kernel space
   */
  template <typename T>
  inline T phys_to_kernel_ptr(T ptr)
  {
    return reinterpret_cast<T>(reinterpret_cast<uint64_t>(ptr) ^ 0xFFFF000000000000);
  }
}

#endif