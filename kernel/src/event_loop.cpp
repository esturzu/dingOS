#include "event_loop.h"
#include "ipc.h"
#include "message.h"

#include "printf.h"
#include "queue.h"

LocklessQueue<Event*>* ready_queue;
LocklessQueue<Message>* msg_queue;

void init_event_loop() { ready_queue = new LocklessQueue<Event*>(); }

void event_loop() {
  while (true) {
    if (!ready_queue->is_empty()) {
      Event* ready_work = ready_queue->dequeue();
      if (ready_work != 0) {
        ready_work->run();
      }
    }
  }
}