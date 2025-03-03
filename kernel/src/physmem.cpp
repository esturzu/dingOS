#include "atomics.h"
#include "stdint.h"
#include "printf.h"

#define PAGE_SIZE 4096  // 4KB
#define TOTAL_MEMORY 0x20000000  // 256MB reserved for pages
#define TOTAL_PAGES (TOTAL_MEMORY / PAGE_SIZE)  // 65536 pages
#define BITMAP_SIZE (TOTAL_PAGES / 8)  // 8192 bytes (2 pages)

// Read briefly about this here: https://wiki.osdev.org/Page_Frame_Allocation#Bitmap
static uint64_t* bitmap;

extern "C" char* _frame_start;
extern "C" char* _frame_end;

// TODO, keep count of current number of pages and then calculate the number of pages needed to
// allocate an entire region (in some heap extend function)? IF not enough pages, then we can 
// auto reject.
namespace PhysMem {

    static SpinLock lock{};

    // Start and end addresses of the page region
    static char* frame_start;
    static char* frame_range_end;


    // Simple method meant to zero out a variable page size, used before handing pages to requests.
    void zero_out(char* page, size_t size) {
        for (int i = 0; i < size; i++) {
            page[i] = 0;
        }
    }

    void* allocate_frame() {

        // To prevent race conditions within the allocation space
        LockGuard<SpinLock> l(lock);

        // Searching the bitmap for the first free page
        for (int i = 0; i < BITMAP_SIZE / 8; i++) {
            // A page is marked 1 in the bitmap if it is allocated, therefore,
            // if the value of the 64 bits gotten from this index is not all 1's,
            // there is a free page.
            if (bitmap[i] != 0xFFFFFFFFFFFFFFFF) {
                // Using 64 bits at a time to make search quicker
                uint64_t word = bitmap[i];

                // Check each bit in the 64 bit chunk to find the first zero
                // (may be a more optimal way to do this)
                for (int offset = 0; offset < 64; offset++) {
                    // Found the free page
                    if ((word & 0b1) == 0) {
                        // Mark as allocated
                        bitmap[i] |= (1ULL << offset);
                        // Get the actual address of the page with some simple math
                        // Start of frame region, plus 64 times the chunk we're on (to represent
                        // the chunks without a free frame) and plus the offset to get the exact index
                        // of the page, now multiply by 4096 to get to the page itself.
                        void* new_page = frame_start + ((i * 64 + offset) * PAGE_SIZE);

                        // Zeroes out the page
                        zero_out((char*) new_page, PAGE_SIZE);
                        debug_printf("Frame found at 0x%X\n", new_page);
                        return new_page;
                    }
                    // Look at the next bit
                    word >> 1;
                }
            }
        }
        debug_printf("No available frames\n");
        return nullptr;
    }

    void free_frame(void* page) {

        uint64_t page_addr = (uint64_t) page;
        debug_printf("Deallocating Addr: 0x%X\n", page_addr);
        if (page_addr % PAGE_SIZE != 0) {
          debug_printf("Attempting to deallocate an address not 4096 Byte Aligned\n");
        }

        // Get the exact number of the page (essentially its index in the bitmap)
        uint64_t page_num = (page_addr - (uint64_t) frame_start) / PAGE_SIZE;

        // Divide the page by 64 to get the chunk it is in
        uint64_t word_idx = page_num / 64;

        // Get the exact bit we are trying to free.
        uint64_t bit_idx = page_num % 64;

        // Marks the page as free in the bitmap
        bitmap[word_idx] &= ~(1ULL << bit_idx);
    }

    void page_init() {

        // Set up actual pointers and values so we don't have to use symbols
        frame_start = (char*) &_frame_start;
        frame_range_end = (char*) &_frame_end;
        size_t frame_size = (size_t) &_frame_end - (size_t) &_frame_start;


        // Bitmap will just be at the start of the first available frame region
        bitmap = (uint64_t*) &_frame_start;

        // Mark all pages in region as freed
        zero_out(frame_start, BITMAP_SIZE);
        
        // Marks the first N pages as used, since these pages are used for the bitmap themselves
        // Simple calculation of the region's size, the number of bits needed to represent each page
        // (i.e. memory space / 4096 == number of bits needed), then divide number off bits by 4096 again
        // to determine how many pages needed.
        for (int i = 0; i < (BITMAP_SIZE / PAGE_SIZE) && i < TOTAL_PAGES; i++) {
            uint64_t word = bitmap[i / 64];
            uint64_t offset = i % 64;
            bitmap[i / 64] |= (1ULL << offset);
            debug_printf("Page %d of bitmap allocated for bitmapping\n", i);
        }
        
        // Set the start of the frame region to after bitmap
        frame_start += BITMAP_SIZE;

        debug_printf("Bitmap Location: 0x%X, Page Start: 0x%X, Page Range End: 0x%X\n", bitmap, frame_start, frame_range_end);
    }
}

// Simple debugging tests right now.
void run_page_tests() {
    // for (int i = 0; i < 100; i++) {
    //     debug_printf("Finished loop %d\n", i);
    //     void* ptr = PhysMem::allocate_frame();
    // }
}