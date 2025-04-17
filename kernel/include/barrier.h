#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "stdint.h"
#include "semaphore.h"
#include "atomics.h"
#include "printf.h"

class Barrier {
    Atomic<int> count;
    Semaphore* sem;
public:
    Barrier(int count) : count(count) {
        sem = new Semaphore(0);
    }

    Barrier(const Barrier&) = delete;

    void sync() {
        if (count.add_fetch(-1) == 0) {
            sem->up();
        }
        else {
            sem->down();
            sem->up();
        }
    }
};

#endif