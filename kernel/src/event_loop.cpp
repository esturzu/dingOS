#include "event_loop.h"

#include "queue.h"
#include "printf.h"



void event_loop(Queue *queue) {
  Debug::printf("Entering event loop!\n");
  // while (!empty(queue)) {
  //   Work event = (Work) dequeue(queue);
  //   event(0);
  // }
}

void event_loop_test() {
  // Rudimentary test
  // Don't have malloc so have to do sketchy stuff
  Queue queue;

  // Node fun2_node = { (void*)fun2, nullptr };
  // Node fun1_node = { (void*)fun1, &fun2_node };
  // queue.head = &fun1_node;

  event_loop(&queue);
}
