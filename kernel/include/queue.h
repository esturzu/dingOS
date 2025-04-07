#ifndef QUEUE_H
#define QUEUE_H

#include "atomics.h"
#include "printf.h"

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
    // Node* current = head;
    // while (current != nullptr) {
    //     Node* next = current->next;
    //     delete current;
    //     current = next;
    // }
  }

  void enqueue(T item) {
    Node* tmp = new Node();
    tmp->item = item;
    tmp->next = nullptr;

    Node* tmp_tail;
    bool successful_exchange;
    do {
      tmp_tail = __atomic_load_n(&tail, __ATOMIC_SEQ_CST);
      Node* tail_next = tmp_tail->next;
      if (tail_next != nullptr) {
        __atomic_compare_exchange_n(&tail, &tmp_tail, tail_next, true,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        successful_exchange = false;
      } else {
        Node* expected = nullptr;
        successful_exchange =
            __atomic_compare_exchange_n(&tmp_tail->next, &expected, tmp, true,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
      }
    } while (!successful_exchange);

    __atomic_compare_exchange_n(&tail, &tmp_tail, tmp, false, __ATOMIC_SEQ_CST,
                                __ATOMIC_SEQ_CST);
  }

  T dequeue() {
    Node* prev_head;
    Node* next;
    do {
      prev_head = __atomic_load_n(&head, __ATOMIC_SEQ_CST);
      next = prev_head->next;
      // printf("Dequeue: head=%p, prev_head=%p, next=%p\n", (void*)head,
      // prev_head, next);
      if (next == nullptr) {
        // printf("Dequeue: queue is empty\n");
        return T();
      }
      // printf("Attempting CAS: head from %p to %p\n", prev_head, next);
      if (__atomic_compare_exchange_n(&head, &prev_head, next, true,
                                      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        // printf("CAS succeeded\n");
        break;
      } else {
        // printf("CAS fa/iled, retrying\n");
      }
    } while (true);
    T item = next->item;
    // delete prev_head;
    return item;
  }

  bool is_empty() {
    return __atomic_load_n(&head, __ATOMIC_SEQ_CST) ==
           __atomic_load_n(&tail, __ATOMIC_SEQ_CST);
  }
};

#endif  // QUEUE_H
