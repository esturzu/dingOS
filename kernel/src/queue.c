#include "queue.h"

void init_queue(Queue *queue) {
  queue->head = queue->tail = NULL;
}

void enqueue(Queue *queue, void *item) {
  // TODO: requires malloc here
}

void *dequeue(Queue *queue) {
  if (queue->head == NULL)
    return NULL;

  // Pop a node
  Node *temp = queue->head;
  queue->head = queue->head->next;
  void *item = temp->item;

  // If head is NULL, reset tail
  if (queue->head == NULL)
    queue->tail = NULL;

  // requires free here
  // free(temp);

  return item;
}
