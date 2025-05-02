// Citations
// https://www.geeksforgeeks.org/typedef-in-c/

#ifndef STDINT_H
#define STDINT_H

#if __has_include_next(<stdint.h>)
  #include_next <stdint.h>
#endif

typedef unsigned char uint8_t;

#ifndef INT8_MAX
typedef char int8_t;
#endif

typedef unsigned short uint16_t;
typedef short int16_t;

typedef unsigned int uint32_t;
typedef int int32_t;

typedef unsigned long uint64_t;
typedef long int64_t;

typedef long unsigned int size_t;

typedef size_t align_val_t;

typedef unsigned long size_t;

typedef unsigned long uintptr_t;

#ifndef INTMAX_MAX
typedef long long intmax_t;
#endif

typedef long long ptrdiff_t;

#endif
