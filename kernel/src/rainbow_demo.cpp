#include "rainbow_demo.h"
#include "event_loop.h"

/* ------------------------------------------------------------------ *
 *  Fast integer “colour-wheel” (0--767) → 0xRRGGBB (ARGB32 is OK too)
 *    0-255 : Red   → Green
 *  256-511 : Green → Blue
 *  512-767 : Blue  → Red
 * ------------------------------------------------------------------ */
static inline uint32_t wheel(uint16_t pos)
{
    pos &= 0x2FF;                 // 0-767
    if (pos < 256) {              // R->G
        return ((255-pos) << 16) | (pos << 8);
    } else if (pos < 512) {       // G->B
        pos -= 256;
        return ( (255-pos) <<  8) |  pos;
    } else {                      // B->R
        pos -= 512;
        return ( pos << 16) | (255-pos);
    }
}

/* ------------------------------------------------------------------ *
 *  Draw one frame: horizontal rainbow that scrolls by “phase”.
 * ------------------------------------------------------------------ */
static void draw_frame(FrameBufferInfo* fb, uint32_t phase)
{
    const uint32_t w = fb->width;
    const uint32_t h = fb->height;
    const uint32_t pitch = fb->pitch;
    uint8_t* base = fb->buffer;

    for (uint32_t y = 0; y < h; ++y) {
        volatile uint32_t* row =
            reinterpret_cast<volatile uint32_t*>(base + y * pitch);
        uint16_t pos = (phase + y) & 0x2FF;       // slight vertical skew
        for (uint32_t x = 0; x < w; ++x) {
            row[x] = wheel(pos);
            pos += 2;                             // 2-pixel colour step
        }
    }
}

/* ------------------------------------------------------------------ *
 *  Every time this lambda runs, it draws the next frame and
 *  re-schedules itself 16 000 µs (~60 fps) later.
 * ------------------------------------------------------------------ */
static void rainbow_task(FrameBufferInfo* fb)
{
    static uint32_t phase = 0;
    draw_frame(fb, phase++);
    /* queue ourselves again in 16 ms */
    schedule_event([fb] { rainbow_task(fb); });
}

/* ------------------------------------------------------------------ */
void start_rainbow_demo(FrameBufferInfo* fb)
{
    rainbow_task(fb);     /* kick it once – the task re-queues itself  */
}
