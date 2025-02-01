#ifndef QUEUE_H
#define QUEUE_H

#include "atomics.h"

template <typename T>
class LocklessQueue
{
  struct Node
  {
    T item;
    Atomic<Node*> next; 
  };

  Atomic<Node*> head;
  Atomic<Node*> tail;

public:
  
  LocklessQueue();
  ~LocklessQueue();
  void enqueue(T item);
  T dequeue();
  bool is_empty();
};

template <typename T>
class Queue
{
  struct Node
  {
    T item; 
    Node* next;
  };

  Node* head;
  Node* tail;

public:

  Queue();
  ~Queue();
  void enqueue(T item);
  T dequeue();
  bool is_empty();
};

#endif // QUEUE_H
