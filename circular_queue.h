#include <pthead.h>

#define MAX_QUEUE_SIZE 1024

typedef item *item;

typedef struct {
    item items[MAX_QUEUE_SIZE];
    unsigned long head;
    unsigned long tail
    unsigned long size;
} queue;


item get_from_queue(queue q);

void add_to_queue(queue q, item i);
