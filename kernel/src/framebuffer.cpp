#include "framebuffer.h"

#include "printf.h"

// -----------------------------------------------------------------------------
// Broadcom BCM2837 addresses
// -----------------------------------------------------------------------------
static constexpr uint32_t PERIPHERAL_BASE = 0x3F000000;
static constexpr uint32_t MAILBOX_BASE = PERIPHERAL_BASE + 0xB880;

static constexpr uint32_t MAILBOX_READ = MAILBOX_BASE + 0x00;
static constexpr uint32_t MAILBOX_STATUS = MAILBOX_BASE + 0x18;
static constexpr uint32_t MAILBOX_WRITE = MAILBOX_BASE + 0x20;

static constexpr uint32_t MAILBOX_STATUS_FULL = (1 << 31);
static constexpr uint32_t MAILBOX_STATUS_EMPTY = (1 << 30);

// Channel number for property interface (GPU property mailbox)
static constexpr uint8_t MAILBOX_CHANNEL_PROP = 8;

// -----------------------------------------------------------------------------
// Simple MMIO (memory-mapped I/O) helpers
// -----------------------------------------------------------------------------
static inline void mmio_write(uint32_t reg, uint32_t data) {
  *reinterpret_cast<volatile uint32_t*>(reg) = data;
}

static inline uint32_t mmio_read(uint32_t reg) {
  return *reinterpret_cast<volatile uint32_t*>(reg);
}

// -----------------------------------------------------------------------------
// A simple mailbox call function that sends an address (must be 16-byte
// aligned) to the given channel, then waits for the response.
// -----------------------------------------------------------------------------
static bool mailbox_call(uint32_t msg_addr, uint8_t channel) {
  // Message buffer address must be 16-byte aligned per the Pi firmware docs
  if (msg_addr & 0xF) {
    return false;
  }

  // Wait until mailbox can accept a new write
  while (mmio_read(MAILBOX_STATUS) & MAILBOX_STATUS_FULL) {
    // Spin
  }

  // Write: the lower 4 bits of data must be the channel, upper bits = address
  mmio_write(MAILBOX_WRITE, (msg_addr & ~0xF) | (channel & 0xF));

  // Wait for the response
  while (true) {
    // Wait while mailbox is empty
    while (mmio_read(MAILBOX_STATUS) & MAILBOX_STATUS_EMPTY) {
      // Spin
    }

    uint32_t data = mmio_read(MAILBOX_READ);
    // If this is our response
    if ((data & 0xF) == channel) {
      // Check if it matches our message address
      if ((data & ~0xF) == (msg_addr & ~0xF)) {
        return true;
      }
    }
  }

  // Shouldn't get here
  // return false;
}

// -----------------------------------------------------------------------------
// Store the allocated framebuffer info in a static variable
// -----------------------------------------------------------------------------
static FrameBufferInfo g_fbInfo;

// -----------------------------------------------------------------------------
// Implementation of framebuffer_init
// Sets up an 800x600 (or user-specified) buffer at a given bpp using
// the VideoCore GPU property interface via mailbox.
// -----------------------------------------------------------------------------
FrameBufferInfo* framebuffer_init(uint32_t width, uint32_t height,
                                  uint32_t bpp) {
  // Align on 16 bytes. Store our tag buffer in a static array so it has a
  // stable address. Mark it volatile so the compiler doesn't optimize out
  // updates.
  static volatile uint32_t __attribute__((aligned(16))) mbox[36];

  // Clear mailbox buffer
  for (int i = 0; i < 36; i++) {
    mbox[i] = 0;
  }

  // Buffer size (in bytes)
  mbox[0] = sizeof(mbox);
  // Request code = 0
  mbox[1] = 0x00000000;

  int idx = 2;
  // Tag: set physical (display) width/height
  mbox[idx++] = 0x48003;  // set_physical_w_h
  mbox[idx++] = 8;        // value size
  mbox[idx++] = 8;        // request size
  mbox[idx++] = width;
  mbox[idx++] = height;

  // Tag: set virtual (framebuffer) width/height
  mbox[idx++] = 0x48004;  // set_virtual_w_h
  mbox[idx++] = 8;
  mbox[idx++] = 8;
  mbox[idx++] = width;
  mbox[idx++] = height;

  // Tag: set depth
  mbox[idx++] = 0x48005;
  mbox[idx++] = 4;
  mbox[idx++] = 4;
  mbox[idx++] = bpp;  // bits per pixel

  // Tag: set pixel order (RGB = 1, BGR = 0)
  mbox[idx++] = 0x48006;
  mbox[idx++] = 4;
  mbox[idx++] = 4;
  mbox[idx++] = 1;  // RGB

  // Tag: allocate framebuffer
  mbox[idx++] = 0x40001;
  mbox[idx++] = 8;
  mbox[idx++] = 8;
  mbox[idx++] = 0;  // GPU returns base address here
  mbox[idx++] = 0;  // GPU returns size here

  // Tag: get pitch
  mbox[idx++] = 0x40008;
  mbox[idx++] = 4;
  mbox[idx++] = 4;
  mbox[idx++] = 0;  // GPU returns pitch here

  // End tag
  mbox[idx] = 0;

  // Call mailbox
  uint32_t mbox_ptr = reinterpret_cast<uintptr_t>(&mbox[0]);
  uint32_t mbox_32 = static_cast<uint32_t>(mbox_ptr);
  if (!mailbox_call(mbox_32, MAILBOX_CHANNEL_PROP)) {
    return nullptr;
  }

  // Check if the firmware returned a success code
  if (mbox[1] != 0x80000000) {
    debug_printf("Mailbox returned code=0x%x (not success)\n", mbox[1]);
    return nullptr;
  }

  // The GPU returns the allocated frame buffer address in the 0x40001 tag data
  // Layout: [tag, valBufSize, reqSize, fbAddr, fbSize]
  // fbAddr = mbox[23]
  // fbSize = mbox[24]
  // pitch is in mbox[28]
  uint32_t fb_addr = mbox[23];
  uint32_t fb_size = mbox[24];
  uint32_t fb_pitch = mbox[28];

  // On real Pi 3 hardware, the returned address may need the top bits cleared
  fb_addr &= 0x3FFFFFFF;

  // Store the info in our static struct
  g_fbInfo.width = width;
  g_fbInfo.height = height;
  g_fbInfo.pitch = fb_pitch;
  g_fbInfo.is_rgb = 1;
  g_fbInfo.buffer = reinterpret_cast<uint8_t*>(fb_addr);
  g_fbInfo.buffer_size = fb_size;

  return &g_fbInfo;
}

// -----------------------------------------------------------------------------
// Implementation of framebuffer_fill
// A quick way to fill the entire screen with a single color.
// This is just an example of direct pixel access (e.g., 32 bpp).
// -----------------------------------------------------------------------------
void framebuffer_fill(FrameBufferInfo* fb, uint32_t color) {
  if (!fb || !fb->buffer || fb->pitch == 0) {
    return;  // Invalid fb
  }

  // Debug print framebuffer data
  debug_printf("Going to fill the framebuffer now!\n");
  debug_printf("fb->width=%u, fb->height=%u, fb->pitch=%u, buffer=0x%x\n",
               fb->width, fb->height, fb->pitch, fb->buffer);

  // Naive row-by-row fill
  for (uint32_t y = 0; y < fb->height; y++) {
    // Pointer to start of this row in the buffer
    volatile uint32_t* rowPtr =
        (volatile uint32_t*)(fb->buffer + y * fb->pitch);

    for (uint32_t x = 0; x < fb->width; x++) {
      rowPtr[x] = color;
    }
  }
}
