#include "event_loop.h"

#include "queue.h"
#include "debug.h"

void event_loop(Queue *queue) {
  debug_print("Entering event loop!\n");
  while (!empty(queue)) {
    Work event = (Work) dequeue(queue);
    event(0);
  }
}

void fun1(void *args) {
  debug_print("In fun1()\n");
}

void fun2(void *args) {
  debug_print("In fun2(), calling fun1()\n");
  fun1(0);
}

void event_loop_test() {
  // Rudimentary test
  // Don't have malloc so have to do sketchy stuff
  Queue queue;

  Node fun2_node = { .item = fun2, .next = 0 };
  Node fun1_node = { .item = fun1, .next = &fun2_node };
  queue.head = &fun1_node;

  event_loop(&queue);
}
