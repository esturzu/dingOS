#ifndef IPC_H
#define IPC_H

#include "message_queue.h"

namespace IPC_MSG {
    void send(int sender, int receiver, MessageType type, void* payload) {
        sendMessage(sender, receiver, type, payload);
    }
    bool recv(int receiver, Message& msg) {
        return receiveMessage(receiver, msg);
    }
}

#endif // IPC_H