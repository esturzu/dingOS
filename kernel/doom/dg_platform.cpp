#include "dg_platform.h"

#include "../include/framebuffer.h"
#include "../include/machine.h"
#include "../include/printf.h"

static uint8_t* g_fb;
static uint32_t g_pitch, g_w, g_h;

void DG_Init(uint32_t w, uint32_t h, uint8_t* fb, uint32_t pitch) {
  g_fb = fb;
  g_pitch = pitch;
  g_w = w;
  g_h = h;
}

extern uint32_t DG_Palette[256];
extern uint8_t DG_ScreenBuffer[];

void DG_SwitchBuffer(void) {
  for (uint32_t y = 0; y < g_h; ++y) {
    uint8_t* src = &DG_ScreenBuffer[y * g_w];
    uint32_t* dst = (uint32_t*)(g_fb + y * g_pitch);
    for (uint32_t x = 0; x < g_w; ++x) {
      uint32_t c = DG_Palette[src[x]];
      dst[x] = 0xFF000000 | (c & 0x00FFFFFF); /* ARGB8888 */
    }
  }
}

void DG_SleepMs(int ms) { delay_ms(ms); }
uint32_t DG_GetTicksMs(void) { return ticks_ms(); }
int DG_GetKey(void) { return 0; }  // stub
void DG_Print(const char* f, ...) { /* optional UART printf */ }

extern const uint8_t _binary_doom1_wad_start[];
extern const uint8_t _binary_doom1_wad_end[];

const uint8_t* DG_LoadWad(size_t* sz) {
  *sz = _binary_doom1_wad_end - _binary_doom1_wad_start;
  return _binary_doom1_wad_start;
}
