#ifndef QUEUE_H
#define QUEUE_H

#include "atomics.h"

template <typename T>
class LocklessQueue {
  struct Node {
    T item;
    Node* next;
  };

  Node* head;
  Node* tail;

 public:
  LocklessQueue() {
    Node* dummy = new Node();
    dummy->item = {};
    dummy->next = 0;

    head = dummy;
    tail = dummy;
  }

  ~LocklessQueue() {
    // Need to Free Queue
  }

  void enqueue(T item) {
    bool successful_exchange;

    Node* tmp = new Node();
    tmp->item = item;
    tmp->next = 0;

    Node* tmp_tail;
    do {
      tmp_tail = __atomic_load_n(&tail, __ATOMIC_SEQ_CST);
      Node* tail_next = tmp_tail->next;
      if (tail_next != 0) {
        __atomic_compare_exchange_n(&tail, &tmp_tail, tail_next, true,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        successful_exchange = false;
      } else {
        successful_exchange = __atomic_compare_exchange_n(
            &tail->next, &tmp->next /*nullptr*/, tmp, true, __ATOMIC_SEQ_CST,
            __ATOMIC_SEQ_CST);
      }
    } while (!successful_exchange);

    __atomic_compare_exchange_n(&tail, &tmp_tail, tmp, false, __ATOMIC_SEQ_CST,
                                __ATOMIC_SEQ_CST);
  }

  T dequeue() {
    Node* prev_head;
    T item;

    do {
      prev_head = __atomic_load_n(&head, __ATOMIC_SEQ_CST);

      // Empty Queue
      if (prev_head->next == 0) return {};

      // A race condition exists here
      item = prev_head->next->item;
    } while (!__atomic_compare_exchange_n(&head, &prev_head, prev_head->next,
                                          true, __ATOMIC_SEQ_CST,
                                          __ATOMIC_SEQ_CST));

    delete prev_head;

    return item;
  }

  bool is_empty() {
    return __atomic_load_n(&head, __ATOMIC_SEQ_CST) ==
           __atomic_load_n(&tail, __ATOMIC_SEQ_CST);
  }
};

#endif  // QUEUE_H
