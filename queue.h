#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

void initQueue(Queue *q);
void enqueue(Queue *, int);
int dequeue(Queue *);
void clearQueue(Queue *);

#endif /* QUEUE_H_INCLUDED */