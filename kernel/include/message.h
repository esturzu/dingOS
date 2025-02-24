#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

// Message Type Enumeration
enum MessageType {
    MSG_SYSTEM_CALL,
    MSG_PROCESS_CREATE,
    MSG_PROCESS_EXIT,
    MSG_DATA,
    MSG_INTERRUPT
};

// Message Structure
struct Message {
    int sender;
    int receiver; // -1 for broadcast
    MessageType type;
    void* load;
};

#endif // MESSAGE_H
