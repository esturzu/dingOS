#include "queue.h"

// Lockless Queue Implementation

template <typename T>
LocklessQueue<T>::LocklessQueue()
{
  head.store(0);
}

template <typename T>
LocklessQueue<T>::~LocklessQueue()
{
  // TODO: requires free here
}

template <typename T>
void LocklessQueue<T>::enqueue(T item)
{
  // TODO: requires malloc here
}

template <typename T>
T LocklessQueue<T>::dequeue()
{
  bool successful_exchange;
  Node* temp;

  do 
  {
    temp = head.load();

    if (temp == 0)
      break;
    else
      successful_exchange = head.compare_exchange(temp, temp->next.load()); // can be improved with better atomic definitions
  } 
  while (!successful_exchange);

  // TODO: Need to free the node

  return {};
}

template <typename T>
bool LocklessQueue<T>::is_empty()
{
  return head.load() == 0;
}

// Queue Implementation

template <typename T>
Queue<T>::Queue()
{
  head = 0;
}

template <typename T>
Queue<T>::~Queue()
{
  // TODO: Need to free the queue
}

template <typename T>
void Queue<T>::enqueue(T item)
{
  // TODO: requires malloc here
}

template <typename T>
T Queue<T>::dequeue()
{
  if (head == 0)
    return {};

  Node* temp = head;
  head = head->next;
  T item = temp->item;

  if (head == 0)
    tail = 0;

  // TODO: Need to free the node

  return item;
}

template <typename T>
bool Queue<T>::is_empty()
{
  return head == 0;
}
