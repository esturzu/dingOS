#ifndef QUEUE_H
#define QUEUE_H

////////////////////////////////////
// A generic queue implementation //
////////////////////////////////////

// Note: only supports single-core, no locks, uses linked list structure
// Todo: change to lock-less queue for multi-cores

typedef struct {
  void *item;
  struct Node *next;
} Node;

typedef struct {
  Node *head;
  Node *tail;
} Queue;

void init_queue(Queue *queue);
void enqueue(Queue *queue, void *item);
void *dequeue(Queue *queue);

#endif // QUEUE_H
