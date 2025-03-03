#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "message.h"
#include "queue.h"
#include "atomics.h"

extern LocklessQueue<Message>* messageQueue; // Pointer instead of object
extern SpinLock lock;

void init_message_queue(); // New initialization function
void sendMessage(int sender, int receiver, MessageType type, void* payload);
bool receiveMessage(int receiver, Message& msg);

#endif // MESSAGE_QUEUE_H