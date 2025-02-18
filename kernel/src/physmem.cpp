#include "atomics.h"
#include "stdint.h"
#include "printf.h"

#define PAGE_SIZE 4096  // 4KB
#define TOTAL_MEMORY 0x10000000  // 256MB reserved for pages
#define TOTAL_PAGES (TOTAL_MEMORY / PAGE_SIZE)  // 65536 pages
#define BITMAP_SIZE (TOTAL_PAGES / 8)  // 8192 bytes (2 pages)

static uint64_t* bitmap;

extern "C" char* _frame_start;
extern "C" char* _frame_end;

namespace PhysMem {

    static SpinLock lock{};

    struct PageFrame {
        PageFrame* next;
    };

    static PageFrame* frame_free_list = nullptr;
    static char* frame_start;
    static char* frame_range_end;

    void page_fill(char* page, size_t size, uint8_t val) {
        for (int i = 0; i < size; i++) {
            page[i] = val;
        }
    }

    void* allocate_frame() {

        LockGuard<SpinLock> l(lock);

        for (int i = 0; i < BITMAP_SIZE / 8; i++) {
            if (bitmap[i] != 0xFFFFFFFFFFFFFFFF) {
                uint64_t word = bitmap[i];
                for (int offset = 0; offset < 64; offset++) {
                    if (word & 0x1 == 0) {
                        bitmap[i] |= (1ULL << offset);
                        void* new_page = frame_start + ((i * 64 + word) * PAGE_SIZE);
                        page_fill((char*) new_page, PAGE_SIZE, 0);
                        dPrintf("Frame found at 0x%X\n", new_page);
                        return new_page;
                    }
                }
            }
        }
        dPrintf("No available frames\n");
        return nullptr;
    }

    void free_frame(void* page) {
        uint64_t page_addr = (uint64_t) page;
        dPrintf("Deallocating Addr: 0x%X\n", page_addr);
        if (page_addr % PAGE_SIZE != 0) {
            dPrintf("Attempting to deallocate an address not 4096 Byte Aligned\n");
        }

        uint64_t page_num = (page_addr - (uint64_t) frame_start) / PAGE_SIZE;
        uint64_t word_idx = page_num / 64;
        uint64_t bit_idx = page_num % 64;

        bitmap[word_idx] &= ~(1ULL << bit_idx);
    }

    void page_init() {
        frame_start = (char*) &_frame_start;
        frame_range_end = (char*) &_frame_end;
        size_t frame_size = (size_t) &_frame_start - (size_t) &_frame_end;

        bitmap = (uint64_t*) &_frame_start;

        page_fill(frame_start, BITMAP_SIZE, 1);
        
        for (int i = 0; i < (BITMAP_SIZE / PAGE_SIZE) && i < TOTAL_PAGES; i++) {
            uint64_t word = bitmap[i / 64];
            uint64_t offset = i % 64;
            bitmap[i / 64] &= !(1ULL << offset);
            dPrintf("Page %d of bitmap allocated for bitmapping\n");
        }
        
        frame_start += BITMAP_SIZE;

        dPrintf("Bitmap Location: 0x%X, Page Start: 0x%X, Page Range End: 0x%X\n", bitmap, &_frame_start, &_frame_end);
    }
}

void run_page_tests() {
    
}