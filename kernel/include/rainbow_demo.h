#pragma once
#include "framebuffer.h"

/* Kick-off a non-blocking, 60 Hz rainbow demo.
 * Call once (after framebuffer_init) and forget about it. */
void start_rainbow_demo(FrameBufferInfo* fb);
