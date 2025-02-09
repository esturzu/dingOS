// Citations
// https://stackoverflow.com/questions/51837684/c-save-lambda-functions-as-member-variables-without-function-pointers-for-opti

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "heap.h"
#include "queue.h"

struct Event {
  // General Event data goes here

  explicit inline Event() {}
  virtual void run() = 0;
  virtual ~Event() {}
};

template <typename Work>
struct EventWithWork : public Event {
  Work const work;

  explicit inline EventWithWork(Work const work) : Event(), work(work) {}
  virtual void run() override { work(); }
};

template <typename Work, typename T>
struct EventWithWorkAndValue : public EventWithWork<Work> {
  T* value;

  explicit inline EventWithWorkAndValue(Work const work)
      : EventWithWork<Work>(work) {}
  virtual void run() override {
    work(value);
    // delete value; need memory management
  }
};

extern LocklessQueue<Event*>* ready_queue;

void init_event_loop();
void event_loop();

template <typename Work>
void schedule_event(Work work) {
  Event* event = new EventWithWork<Work>(work);
  ready_queue->enqueue(event);
}

#endif  // EVENT_LOOP_H
