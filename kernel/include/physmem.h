#ifndef _MEM_h_
#define _MEM_h_

#include "stdint.h"

#define PAGE_SIZE 4096

namespace PMem {
    void* allocate_frame();
    void free_frame(void* page); 
    void page_init();
}
void run_page_tests();

#endif