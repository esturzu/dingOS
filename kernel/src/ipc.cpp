#include "printf.h"
#include "atomics.h"
#include "queue.h"
#include "message.h"

LocklessQueue<Message> *msg_queue;

namespace IPC_MSG {

    void send(int sender, int receiver, void *load) {


        


    }

    void recv(int receiver, void *recv_load) {


    }






}