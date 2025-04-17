#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

#include <stdint.h>

/**
 * Holds basic information about the allocated framebuffer
 * after a successful initialization via the mailbox.
 */
struct FrameBufferInfo {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;   // Bytes per row
  uint32_t is_rgb;  // 0 = BGR, 1 = RGB
  uint8_t* buffer;  // Pointer to the start of the framebuffer in ARM addr space
  uint32_t buffer_size;  // Total size in bytes of the allocated framebuffer
};

/**
 * @brief Initialize the HDMI/graphics framebuffer via the mailbox interface.
 *
 * On success, returns a pointer to a static FrameBufferInfo struct describing
 * the allocated buffer (width, height, pitch, pointer to memory, etc.). On
 * failure, returns nullptr.
 *
 * @param width   Desired resolution width
 * @param height  Desired resolution height
 * @param bpp     Bits per pixel (common values: 16, 24, 32)
 *
 */
FrameBufferInfo* framebuffer_init(uint32_t width, uint32_t height,
                                  uint32_t bpp);

/**
 * @brief Fill the entire framebuffer with a single 32-bit color (e.g., 0xFF0000
 * for red).
 *
 * @param fb Pointer to a valid FrameBufferInfo struct (from framebuffer_init).
 * @param color 32-bit color value.
 */
void framebuffer_fill(FrameBufferInfo* fb, uint32_t color);

#endif  // _FRAMEBUFFER_H_
