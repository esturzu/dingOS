#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "atomics.h"
#include "stdint.h"
#include "queue.h"
#include "thread.h"

class Semaphore {
    uint64_t volatile count;
    SpinLock lock;
    LocklessQueue<TCB*> waiting_queue;
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
            thread_block(waiting_queue, lock);
        }
    }

    // Up operation (release semaphore and unblock a thread)
    void up() {
        lock.lock();
        if (!waiting_queue.is_empty()) {
            // Unblock the next thread from the per-semaphore queue
            TCB* next = waiting_queue.dequeue();
            if (next != nullptr)  {
                count++;
            }
            thread_unblock(next);
            lock.unlock();
        } 
    }

};

#endif
