#ifndef _FUTURE_H_
#define _FUTURE_H_

#include "semaphore.h"
#include "stdint.h"

template <typename T>
class Future {
    Semaphore sem;
    bool volatile is_ready;
    T volatile t;
public:
    Future() : sem(), is_ready(false), t() {}

    Future(const Future&) = delete;
    Future& operator=(const Future& rhs) = delete;
    Future& operator=(Future&& rhs) = delete;

    void set(T val) {
        if (!is_ready) {
            t = val;
            is_ready = true;
            sem.up();
        }
    }

    T get() {
        if (!is_ready) {
            sem.down();
            sem.up()
        }
        return t;
    }
};

#endif