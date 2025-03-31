#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "stdint.h"
#include "semaphore.h"
#include "atomics.h"
#include "printf.h"

class Barrier {
    Atomic<int> count;
    Semaphore sem;
public:
    Barrier(uint64_t count) : count(count), sem(0) {}

    Barrier(const Barrier&) = delete;

    void sync() {
        int r = count.add_fetch(-1);
        if (r < 0) {
            debug_printf("TOO MANY THREADS IN BARRIER!\n");
        }
        if (r == 0) {
            sem.up();
        }
        else {
            sem.down();
            sem.up();
        }
    }
};

#endif