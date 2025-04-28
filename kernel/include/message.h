// message.h
#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

enum MessageType {
    MSG_SYSTEM_CALL,
    MSG_PROCESS_CREATE,
    MSG_PROCESS_EXIT,
    MSG_DATA,
    MSG_INTERRUPT
};

struct Message {
    int sender;
    int receiver; // -1 for broadcast
    MessageType type;
    void* payload; // Renamed from 'load' for clarity
};

#endif // MESSAGE_H