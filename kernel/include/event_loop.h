#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

// Dependencies
struct Queue;

typedef void (*Work)(void *);

typedef struct {
  Work work;
} Event;

void event_loop(struct Queue *queue);
void event_loop_test();

#endif // EVENT_LOOP_H
