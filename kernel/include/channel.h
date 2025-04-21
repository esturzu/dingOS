#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "semaphore.h"
#include "thread.h"

template <typename T>
class Channel {
    T message;
    Semaphore sem;
    LocklessQueue<TCB*> send_queue;
    LocklessQueue<TCB*> recv_queue;
public:
    Channel(T msg) : message(msg), sem(1), send_queue(), recv_queue() {}

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel& rhs) = delete;
    Channel& operator=(Channel&& rhs) = delete;

    void send(const T& msg) {
        sem.down();
    }

    T receive() {
        
    }
};

#endif