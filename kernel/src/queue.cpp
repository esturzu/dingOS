#include "queue.h"

void init_queue(Queue *queue) {
  queue->head = queue->tail = 0;
}

void enqueue(Queue *queue, void *item) {
  // TODO: requires malloc here
}

void *dequeue(Queue *queue) {
  if (queue->head == 0)
    return 0;

  // Pop a node
  Node *temp = queue->head;
  queue->head = (Node *) queue->head->next;
  void *item = temp->item;

  // If head is NULL, reset tail
  if (queue->head == 0)
    queue->tail = 0;

  // requires free here
  // free(temp);

  return item;
}

int empty(Queue *queue) {
  return queue->head == 0;
}
