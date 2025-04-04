#include "event_loop.h"

#include "cores.h"
#include "machine.h"
#include "printf.h"
#include "queue.h"

LocklessQueue<Event*>* ready_queue;

extern "C" uint8_t* stack0_top;
extern "C" uint8_t* stack1_top;
extern "C" uint8_t* stack2_top;
extern "C" uint8_t* stack3_top;

void init_event_loop() { ready_queue = new LocklessQueue<Event*>(); }

[[noreturn]]
void event_loop() {

  switch (SMP::whichCore())
  {
    case 0:
      set_stack_pointer(stack0_top);
      break;
    case 1:
      set_stack_pointer(stack1_top);
      break;
    case 2:
      set_stack_pointer(stack2_top);
      break;
    case 3:
      set_stack_pointer(stack3_top);
      break;
  };

  while (true) {
    if (!ready_queue->is_empty()) {
      Event* ready_work = ready_queue->dequeue();
      if (ready_work != 0) {
        ready_work->run();
      }
    }
  }
}