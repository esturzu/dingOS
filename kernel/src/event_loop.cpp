#include "event_loop.h"

#include "queue.h"
#include "printf.h"

LocklessQueue<Event*>* ready_queue;

void init_event_loop()
{
  ready_queue = new LocklessQueue<Event*>();
}

void event_loop()
{
  while (true)
  {
    if (!ready_queue->is_empty())
    {
      Event* ready_work = ready_queue->dequeue();
      if (ready_work != 0)
      {
        ready_work->run();
      }
    }
  }
}