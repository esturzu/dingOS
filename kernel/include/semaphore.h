#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "atomics.h"
#include "stdint.h"
#include "queue.h"
#include "process.h"
#include "event_loop.h"
#include "cores.h"

class Semaphore {
    uint64_t volatile count;
    SpinLock lock;
    LocklessQueue<Process*> waiting_queue;
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
            block();
        }
    }

    // Up operation (release semaphore and unblock a thread)
    void up() {
        lock.lock();
        if (!waiting_queue.is_empty()) {
            // Unblock the next thread from the per-semaphore queue
            Process* next = waiting_queue.dequeue();
            if (next != nullptr)  {
                count++;
            }
            unblock(next);
            lock.unlock();
        } 
    }

    void block() {
        waiting_queue.enqueue(activeProcess[SMP::whichCore()]);
        activeProcess[SMP::whichCore()] = nullptr;
        event_loop();
    }

    void unblock(Process* next) {
        schedule_event([next](){
            next->run();
        });
    }

};

#endif
