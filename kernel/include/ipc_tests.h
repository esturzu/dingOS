#ifndef IPC_TESTS_H
#define IPC_TESTS_H

#include "queue.h"
#include "printf.h"
#include "message.h"
#include "ipc.h"

// Define the test with scheduling send and receive events
void ipcTests() {
    int sender = 1;
    int receiver = 2;
    const char* payload = "Test Message";

    // printf("IPC Tests: Scheduling Test send\n");
    schedule_event([=] {
        // printf("IPC Tests: Starting send event\n");
        IPC_MSG::send(sender, receiver, MSG_DATA, (void*)payload);
        // printf("IPC Tests: Finished send event\n");
    });

    // printf("IPC Tests: Scheduling Test receive\n");
    schedule_event([=] {
        printf("IPC Tests: Receiving message\n");
        Message msg;
        while (!IPC_MSG::recv(receiver, msg)) {
            // printf("IPC Tests: No message received yet, retrying...\n");
            // Optional: Add a small delay to prevent tight looping
            // For example, in a real system, you might yield or sleep briefly
        }
        printf("IPC Test Passed: Received message from %d to %d with type %d\n",
               msg.sender, msg.receiver, msg.type);
    });
} 


#endif  // IPC_TESTS_H