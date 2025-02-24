#include "message_queue.h"

LocklessQueue<Message> messageQueue; 
SpinLock lock{};  

void sendMessage(int sender, int receiver, MessageType type, void* payload) {
    LockGuard<SpinLock> l(lock);
    messageQueue.enqueue({sender, receiver, type, payload});
}

bool receiveMessage(int receiver, Message& msg) {
    LockGuard<SpinLock> l(lock);
    if (messageQueue.is_empty()) return false;

    LocklessQueue<Message> tempQueue;
    bool found = false;

    while (!messageQueue.is_empty()) {
        Message m = messageQueue.dequeue();
        if (m.receiver == receiver && !found) {
            msg = m;
            found = true;
        } else {
            tempQueue.enqueue(m);
        }
    }

    // Restore remaining messages
    while (!tempQueue.is_empty()) {
        messageQueue.enqueue(tempQueue.dequeue());
    }
    return found;
}