#ifndef _threads_h_
#define _threads_h_

#include "queue.h"
#include "functional"
#include "atomics.h"

extern "C" void save_context(TCB* tcb);
extern "C" void restore_context(TCB* tcb);

enum class ThreadState {
    READY,       // Thread is ready to run
    BLOCKED,     // Thread is blocked (waiting on a resource)
    RUNNING,     // Thread is currently executing
    IDLE         // Thread is idle (not doing anything)
};

struct TCB {
    ThreadState state;
    char* stack;       // Pointer to allocated stack memory
    void* savedSP;     // Stores the stack pointer when switching out
    std::function<void()> task;  // The function to be executed by the thread

    // Constructor for initializing TCB
    TCB(std::function<void()> task_func) : state(ThreadState::READY), task(task_func) {
        stack = new char[8192];   // Allocate stack
        savedSP = stack + 8192;   // Stack grows downward
    }

    ~TCB() {
        delete[] stack;
    }
};

// Global ready queue to store all ready threads
LocklessQueue<TCB*> userQueue;

// Currently running thread (initially NULL or IDLE thread)
TCB* currentThread = nullptr;

void schedule_event(std::function<void()> work) {
    // Function to add events (tasks) to the ready queue
    userQueue.enqueue(new TCB(work));
}

// Basic thread context switch (stub for illustration purposes)
void context_switch(TCB* from, TCB* to) {
    save_context(from);
    restore_context(to);
}

// Basic thread yield (for voluntary yielding)
void thread_yield() {
    if (currentThread != nullptr) {
        // Mark current thread as ready
        currentThread->state = ThreadState::READY;

        // Place it back in the ready queue
        userQueue.enqueue(currentThread);

        // Pick the next thread to run
        if (!userQueue.is_empty()) {
            TCB* next = userQueue.dequeue();
            next->state = ThreadState::RUNNING;

            // Perform a context switch
            context_switch(currentThread, next);
        }
    }
}

// Block a thread and place it on a target queue (specific semaphore queue)
void thread_block(LocklessQueue<TCB*>& target_queue, SpinLock& lock) {
    if (currentThread != nullptr) {
        // Mark current thread as blocked
        currentThread->state = ThreadState::BLOCKED;

        // Place it on the target queue (per-semaphore queue)
        target_queue.enqueue(currentThread);

        // Switch to another ready thread
        thread_yield();
    }
}

// Basic unblock function (simulates unblocking a thread)
void thread_unblock(TCB* thread) {
    if (thread != nullptr) {
        // Mark the thread as ready
        thread->state = ThreadState::READY;

        // Place it in the ready queue
        userQueue.enqueue(thread);
    }
}

#endif