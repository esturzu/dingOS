#include "framebuffer.h"

#include "definitions.h"
#include "printf.h"
#include "vmm.h"

/* -------------------------------------------------------------------------
 *  Physical addresses of the mailbox registers
 * ------------------------------------------------------------------------- */
static constexpr uint64_t PHYS_MASK = 0x0000FFFFFFFFFFFFULL;
static constexpr uint64_t PERIPH_PHYS =
    PERIPHERALS_BASE & PHYS_MASK;  // 0x4000_0000
static constexpr uint64_t MAILBOX_PHYS = PERIPH_PHYS + 0xB880;
static constexpr uint64_t MAILBOX_READ = MAILBOX_PHYS + 0x00;
static constexpr uint64_t MAILBOX_STATUS = MAILBOX_PHYS + 0x18;
static constexpr uint64_t MAILBOX_WRITE = MAILBOX_PHYS + 0x20;

static constexpr uint32_t MAILBOX_STATUS_FULL = (1u << 31);
static constexpr uint32_t MAILBOX_STATUS_EMPTY = (1u << 30);
static constexpr uint8_t MAILBOX_CHANNEL_PROP = 8;

/* ---------- helpers: PHYS → kernel‑virtual and simple MMIO I/O ------------ */
static inline volatile uint32_t* kva(uint64_t phys) {
  return VMM::phys_to_kernel_ptr(reinterpret_cast<volatile uint32_t*>(phys));
}
static inline void mmio_write(uint64_t phys, uint32_t v) { *kva(phys) = v; }
static inline uint32_t mmio_read(uint64_t phys) { return *kva(phys); }

/* ---------- simple cache maintenance helpers (AArch64) -------------------- */
static inline void dcache_clean(const void* p, size_t len) {
  uint64_t addr = (reinterpret_cast<uint64_t>(p)) & ~63ULL;
  uint64_t end = reinterpret_cast<uint64_t>(p) + len;
  for (; addr < end; addr += 64) __asm__ volatile("dc cvac, %0" ::"r"(addr));
}
static inline void dcache_inval(const void* p, size_t len) {
  uint64_t addr = (reinterpret_cast<uint64_t>(p)) & ~63ULL;
  uint64_t end = reinterpret_cast<uint64_t>(p) + len;
  for (; addr < end; addr += 64) __asm__ volatile("dc ivac, %0" ::"r"(addr));
  __asm__ volatile("dsb sy");
  __asm__ volatile("isb");
}

/* ---------- convert ARM‑physical → VideoCore bus --------------- */
static inline uint32_t cpu_to_bus(uint32_t phys) { return phys | 0x40000000u; }

static FrameBufferInfo g_fbInfo;

static bool mailbox_call(uint32_t buf_bus, uint8_t chan) {
  if (buf_bus & 0xF) return false;  // buffer must be 16‑byte aligned

  // make sure GPU sees our tag writes
  dcache_clean(
      VMM::phys_to_kernel_ptr(reinterpret_cast<void*>(buf_bus & ~0x40000000u)),
      256);

  while (mmio_read(MAILBOX_STATUS) & MAILBOX_STATUS_FULL);
  mmio_write(MAILBOX_WRITE, (buf_bus & ~0xF) | (chan & 0xF));

  while (true) {
    while (mmio_read(MAILBOX_STATUS) & MAILBOX_STATUS_EMPTY);
    uint32_t resp = mmio_read(MAILBOX_READ);
    if ((resp & 0xF) == chan && (resp & ~0xF) == (buf_bus & ~0xF)) break;
  }

  // GPU has written the response – invalidate our view of the buffer
  dcache_inval(
      VMM::phys_to_kernel_ptr(reinterpret_cast<void*>(buf_bus & ~0x40000000u)),
      256);
  return true;
}

FrameBufferInfo* framebuffer_init(uint32_t w, uint32_t h, uint32_t bpp) {
  static volatile uint32_t __attribute__((aligned(16))) mbox[36];

  mbox[0] = sizeof(mbox);
  mbox[1] = 0x00000000;
  int i = 2;
  mbox[i++] = 0x48003;
  mbox[i++] = 8;
  mbox[i++] = 8;
  mbox[i++] = w;
  mbox[i++] = h;
  mbox[i++] = 0x48004;
  mbox[i++] = 8;
  mbox[i++] = 8;
  mbox[i++] = w;
  mbox[i++] = h;
  mbox[i++] = 0x48005;
  mbox[i++] = 4;
  mbox[i++] = 4;
  mbox[i++] = bpp;
  mbox[i++] = 0x48006;
  mbox[i++] = 4;
  mbox[i++] = 4;
  mbox[i++] = 1;
  mbox[i++] = 0x40001;
  mbox[i++] = 8;
  mbox[i++] = 8;
  mbox[i++] = 0;
  mbox[i++] = 0;
  mbox[i++] = 0x40008;
  mbox[i++] = 4;
  mbox[i++] = 4;
  mbox[i++] = 0;
  mbox[i] = 0;

  uint32_t buf_phys = reinterpret_cast<uintptr_t>(&mbox[0]) & 0xFFFFFFFF;
  uint32_t buf_bus = cpu_to_bus(buf_phys);

  if (!mailbox_call(buf_bus, MAILBOX_CHANNEL_PROP) || mbox[1] != 0x80000000) {
    debug_printf("mailbox failed: 0x%x\n", mbox[1]);
    return nullptr;
  }

  uint32_t fb_bus = mbox[23];
  uint32_t fb_size = mbox[24];
  uint32_t fb_pitch = mbox[28];

  uint32_t fb_phys = fb_bus & 0x3FFFFFFF;
  uint8_t* fb_kva =
      VMM::phys_to_kernel_ptr(reinterpret_cast<uint8_t*>(fb_phys));

  g_fbInfo = {w, h, fb_pitch, 1, fb_kva, fb_size};
  return &g_fbInfo;
}

void framebuffer_fill(FrameBufferInfo* fb, uint32_t colour) {
  if (!fb || !fb->buffer || !fb->pitch) return;

  for (uint32_t y = 0; y < fb->height; ++y) {
    volatile uint32_t* row =
        reinterpret_cast<volatile uint32_t*>(fb->buffer + y * fb->pitch);
    for (uint32_t x = 0; x < fb->width; ++x) row[x] = colour;
  }
}
