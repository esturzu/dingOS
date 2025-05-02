#ifndef _DG_PLATFORM_H_
#define _DG_PLATFORM_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void DG_Init(uint32_t w, uint32_t h, uint8_t* fb, uint32_t pitch);
void DG_SwitchBuffer(void);
void DG_SleepMs(int ms);
uint32_t DG_GetTicksMs(void);
int DG_GetKey(void);
void DG_Print(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif  // _DG_PLATFORM_H_
