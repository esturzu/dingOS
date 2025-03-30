#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "atomics.h"
#include "stdint.h"
#include "queue.h"
#include "event_loop.h"

class Semaphore {
    uint64_t volatile count;
    SpinLock lock;
    LocklessQueue<Event*>* waiting_queue;
public: 
    Semaphore(const uint32_t count) : count(count), lock(), waiting_queue() {}

    Semaphore(const Semaphore&) = delete;

    void down() {
        lock.lock();

        if (count > 0) {
            count--;
            lock.unlock();
        }

        else {
            waiting_queue->enqueue()
        }

    }

    void up() {
        


    }
}



#endif
