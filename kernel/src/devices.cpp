// Citations
// https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html
// https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html#sect-property-values

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

uint64_t DTB_Property::to_uint64_t()
{
  uint64_t upper_half = __builtin_bswap32(*(static_cast<uint32_t*>(address) + 2));
  uint64_t lower_half = __builtin_bswap32(*(static_cast<uint32_t*>(address) + 3));
  return (upper_half << 32) + lower_half;
}

uint32_t DTB_Property::to_uint32_t()
{
  return __builtin_bswap32(*(static_cast<uint32_t*>(address) + 2));
}

bool DTB_Property::str_equal(const char* other)
{
  struct_property* property = static_cast<struct_property*>(address);
  char* string_value = reinterpret_cast<char*>(property + 1);
  for (uint32_t offset = 0; offset < property->length; offset++)
  {
    if (other[offset] == '\0')
      return false;
    
    if (other[offset] != string_value[offset])
      return false;
  }
  return other[property->length] == '\0';
}

void* get_device(const char* device_path)
{
  DTB_Header* header = (DTB_Header*) dtb_location;

  if (__builtin_bswap32(header->magic) != 0xd00dfeed)
  {
    return 0;
  }

  uint32_t* current_token = (uint32_t*) (((uint8_t*) dtb_location) + __builtin_bswap32(header->structure_block_offset));
  
  char* matched_device_name = (char*) device_path;
  uint32_t wrong_path_depth = 0;

  while (__builtin_bswap32(*current_token) != (uint32_t)FDT_TOKEN_TYPE::FDT_END)
  {
    FDT_TOKEN_TYPE token_type = static_cast<FDT_TOKEN_TYPE>(__builtin_bswap32(*current_token));
    switch (token_type)
    {
      case FDT_TOKEN_TYPE::FDT_BEGIN_NODE:
        {
          // TODO: Replace this with a function for strlen
          char* current_char = (char*)(current_token + 1);
          uint32_t size = 1;

          bool on_device_path = true;

          while (*current_char != '\0')
          {
            if (wrong_path_depth != 0 || matched_device_name[size - 1] == '\0' || matched_device_name[size - 1] == '/' || *current_char != matched_device_name[size - 1])
              on_device_path = false;

            size += 1;
            current_char += 1;
          }

          if (on_device_path)
          {
            matched_device_name = matched_device_name + size;
            if (*matched_device_name == '\0')
              return current_token;
          }
          else
          {
            wrong_path_depth += 1;
          }

          current_token += 1 + ((3 + size) / 4); // Allign to 32 bit boundary
        }
        break;
      case FDT_TOKEN_TYPE::FDT_END_NODE:
        {
          if (wrong_path_depth != 0) wrong_path_depth -= 1; 
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_PROP:
        {
          struct_property* property = (struct_property*) (current_token + 1);
          current_token += 3 + ((3 + __builtin_bswap32(property->length)) / 4); // Allign to 32 bit boundary
        }
        break;
      case FDT_TOKEN_TYPE::FDT_NOP:
        {
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_END:
      default:
        return 0;
    }
  }

  debug_printf("Unable to Find Device: %s\n", device_path);
  return 0;
}

DTB_Property get_device_property(void* device_blob, const char* property_name)
{
  DTB_Header* header = (DTB_Header*) dtb_location;

  if (__builtin_bswap32(header->magic) != 0xd00dfeed)
  {
    return {0};
  }

  uint32_t* current_token = (uint32_t*) device_blob;
  while (__builtin_bswap32(*current_token) != (uint32_t)FDT_TOKEN_TYPE::FDT_END)
  {
    FDT_TOKEN_TYPE token_type = static_cast<FDT_TOKEN_TYPE>(__builtin_bswap32(*current_token));
    switch (token_type)
    {
      case FDT_TOKEN_TYPE::FDT_BEGIN_NODE:
        {
          // TODO: Replace this with a function for strlen
          char* current_char = (char*)(current_token + 1);
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
          debug_printf("Unable to find device property %s\n", property_name);
          return 0;
        }
      case FDT_TOKEN_TYPE::FDT_PROP:
        {
          struct_property* property = (struct_property*) (current_token + 1);
          
          char* found_property_name = ((char*) dtb_location) + __builtin_bswap32(header->string_block_offset) + __builtin_bswap32(property->name_offset);
          uint32_t name_offset = 0;

          // TODO: Replace this with a function for cmp string
          while (1)
          {
            if (property_name[name_offset] != found_property_name[name_offset])
              break;
            else if (property_name[name_offset] == '\0' && found_property_name[name_offset] == '\0')
              return {property};
            else
              name_offset += 1;
          }

          current_token += 3 + ((3 + __builtin_bswap32(property->length)) / 4); // Allign to 32 bit boundary
        }
        break;
      case FDT_TOKEN_TYPE::FDT_NOP:
        {
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_END:
      default:
          return {0};
    }
  }

  debug_printf("Unable to find device property %s\n", property_name);
  return {0};
}

void init_devices()
{
  DTB_Header* header = (DTB_Header*) dtb_location;

  debug_printf("DTB Located At: 0x%p\n", header);

  if (__builtin_bswap32(header->magic) != 0xd00dfeed)
  {
    debug_printf("Magic in DTB is Incorrect (Exepected: 0xd00dfeed, Got:%x)\n", __builtin_bswap32(header->magic));
    return;
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
          debug_printf("Start New Node (Name: %s)\n", current_char);
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
          debug_printf("Current Node Ended\n");
          current_token += 1;
        }
        break;
      case FDT_TOKEN_TYPE::FDT_PROP:
        {
          struct_property* property = (struct_property*) (current_token + 1);
          debug_printf("Property: %s (len:%lu) %p\n", ((char*) dtb_location) + __builtin_bswap32(header->string_block_offset) + __builtin_bswap32(property->name_offset), __builtin_bswap32(property->length), property + 1);
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
          debug_printf("Encountered End Without Escaping First\n");
        }
        break;
      default:
        {
          debug_printf("Unknown DTB Structure Token Encountered (Value 0x%x)\n", __builtin_bswap32(*current_token));
          return;
        }
    }
  }
}

}