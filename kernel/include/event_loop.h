// Citations
// https://stackoverflow.com/questions/51837684/c-save-lambda-functions-as-member-variables-without-function-pointers-for-opti

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "queue.h"

extern void event_loop();

struct Event
{
  virtual void run() = 0;
  virtual ~Event();
};

#endif // EVENT_LOOP_H
