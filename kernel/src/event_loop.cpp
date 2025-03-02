#include "event_loop.h"

#include "printf.h"
#include "queue.h"
#include "cores.h"

LocklessQueue<Event*>* ready_queue;

void init_event_loop() { ready_queue = new LocklessQueue<Event*>(); }

void event_loop() {
  uint32_t i = 0;
  while (true) {
    if (!ready_queue->is_empty()) {
      i = 0;
      Event* ready_work = ready_queue->dequeue();
      if (ready_work != 0) {
        ready_work->run();
      }
    }else{
      if(i++ > 10000){
        i = 0;
        
        debug_printf("Idle Core %d!\n", SMP::whichCore());
      }

    }
  }
}