#include "message_queue.h"
#include "printf.h"

LocklessQueue<Message>* messageQueue = nullptr;
SpinLock lock;

void init_message_queue() {
    if (messageQueue == nullptr) {
        messageQueue = new LocklessQueue<Message>();
        printf("Message queue explicitly initialized\n");
    }
}

void sendMessage(int sender, int receiver, MessageType type, void* payload) {
    // printf("SendMessage: Acquiring lock\n");
    LockGuard<SpinLock> l(lock);
    // printf("SendMessage: Lock acquired\n");
    // printf("Trying to send message from %d to %d\n", sender, receiver);
    if (messageQueue == nullptr) {
        printf("Error: messageQueue not initialized\n");
        return;
    }
    Message msg = {sender, receiver, type, payload};
    messageQueue->enqueue(msg);
    // printf("Enqueued message from %d to %d\n", sender, receiver);
}

bool receiveMessage(int receiver, Message& msg) {
    // printf("ReceiveMessage: Acquiring lock\n");
    LockGuard<SpinLock> l(lock);
    // printf("ReceiveMessage: Lock acquired\n");
    // printf("Trying to receive message for %d\n", receiver);
    if (messageQueue->is_empty()) return false;

    LocklessQueue<Message> tempQueue;
    bool found = false;

    while (!messageQueue->is_empty()) {
        // printf("Trying to receive message for %d\n", receiver);
        Message m = messageQueue->dequeue();
        // printf("Finished dequeuing message\n");
        if (m.receiver == receiver && !found) {
            
            msg = m;
            found = true;
        } else {
            tempQueue.enqueue(m);
        }
    }

    // Restore remaining messages
    while (!tempQueue.is_empty()) {
        messageQueue->enqueue(tempQueue.dequeue());
    }
    return found;
}