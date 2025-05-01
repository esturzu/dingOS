#ifndef DEVICES_H
#define DEVICES_H

#include "stdint.h"

namespace Devices
{

class DTB_Property
{

  void* address;

public:

  DTB_Property(void* addr) : address(addr) {}

  uint64_t to_uint64_t();
  uint32_t to_uint32_t();
  bool str_equal(const char* other);
};

extern void* get_device(const char* device_path);
extern DTB_Property get_device_property(void* device_blob, const char* property_name);
extern void init_devices();

}

#endif