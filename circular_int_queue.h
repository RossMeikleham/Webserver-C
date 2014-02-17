#ifndef CIRCULAR_H
#define CIRCULAR_H

#define MAX_QUEUE_SIZE 1024

typedef struct int_queue queue;

queue *initialize_queue();

int pop_from_queue(queue *q);

void push_to_queue(queue *q, int i);


#endif /* CIRCULAR_H */
