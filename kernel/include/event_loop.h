#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

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
  virtual void run() override {
    work();
  }
};

template <typename Work, typename T>
struct EventWithWorkAndValue : public EventWithWork<Work> {
  T *value;

  explicit inline EventWithWorkAndValue(Work const work) : EventWithWork<Work>(work) {}
  virtual void run() override {
    work(value);
     // delete value; need memory management
  }
};

void event_loop(struct Queue *queue);
void event_loop_test();

#endif // EVENT_LOOP_H
