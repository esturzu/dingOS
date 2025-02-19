// Citations
// https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html

#include "devices.h"
#include "printf.h"

uint64_t dtb_location = 0;

namespace Devices
{

enum class FDT_TOKEN_TYPE : uint32_t
{
  FDT_BEGIN_NODE = 1,
  FDT_END_NODE = 2,
  FDT_PROP = 3,
  FDT_NOP = 4,
  FDT_END = 9
};

struct DTB_Header
{
  uint32_t magic;
  uint32_t device_tree_size;
  uint32_t structure_block_offset;
  uint32_t string_block_offset;
  uint32_t reservation_block_offset;
  uint32_t version;
  uint32_t last_compatible_version;
  uint32_t physical_id_boot_cpu;
  uint32_t string_block_size;
  uint32_t structure_block_size;
};

struct reserve_entry
{
  uint64_t address;
  uint64_t size;
};

struct struct_property
{
  uint32_t length;
  uint32_t name_offset;
};

void init_devices()
{
  DTB_Header* header = (DTB_Header*) dtb_location;

  printf("DTB Located At: 0x%p\n", header);

  if (__builtin_bswap32(header->magic) != 0xd00dfeed)
  {
    printf("Magic in DTB is Incorrect (Exepected: 0xd00dfeed, Got:%x)\n", __builtin_bswap32(header->magic));
  }

  reserve_entry* rsv_entry = (reserve_entry*) (((uint8_t*) dtb_location) + __builtin_bswap32(header->reservation_block_offset));
  while (rsv_entry->address != 0 || rsv_entry->size != 0)
  {
    printf("Found Reserved Memory At (Location: 0x%p, Size: 0x%x)\n", __builtin_bswap64(rsv_entry->address), __builtin_bswap64(rsv_entry->size));
    rsv_entry += 1;
  }

  uint32_t* current_token = (uint32_t*) (((uint8_t*) dtb_location) + __builtin_bswap32(header->structure_block_offset));
  while (__builtin_bswap32(*current_token) != (uint32_t)FDT_TOKEN_TYPE::FDT_END)
  {
    FDT_TOKEN_TYPE token_type = static_cast<FDT_TOKEN_TYPE>(__builtin_bswap32(*current_token));
    switch (token_type)
    {
      case FDT_TOKEN_TYPE::FDT_BEGIN_NODE:
        {
          // TODO: Replace this with a function for strlen
          char* current_char = (char*)(current_token + 1);
          printf("Start New Node (Name: %s)\n", current_char);
          uint32_t size = 1;
          while (*current_char != '\0')
          {
            size += 1;
            current_char += 1;
          }
          current_token += 1 + ((3 + size) / 4); // Allign to 32 bit boundary
        }
        break;
      case FDT_TOKEN_TYPE::FDT_END_NODE:
        {
          printf("Current Node Ended\n");
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_PROP:
        {
          struct_property* property = (struct_property*) (current_token + 1);
          printf("Property: %s\n", ((char*) dtb_location) + __builtin_bswap32(header->string_block_offset) + __builtin_bswap32(property->name_offset));
          current_token += 3 + ((3 + __builtin_bswap32(property->length)) / 4); // Allign to 32 bit boundary
        }
        break;
      case FDT_TOKEN_TYPE::FDT_NOP:
        {
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_END:
        {
          printf("Encountered End Without Escaping First\n");
        }
        break;
      default:
        {
          printf("Unknown DTB Structure Token Encountered (Value 0x%x)\n", __builtin_bswap32(*current_token));
        }
    }
  }
  printf("Done\n");
}

}