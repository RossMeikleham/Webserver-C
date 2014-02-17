/*
 * =====================================================================================
 *
 *       Filename:  client_t_pool.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  15/02/14 18:17:02
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "circular_int_queue.h"

struct int_queue {
    
    int items[MAX_QUEUE_SIZE];
    unsigned long head;
    unsigned long tail;
    unsigned long size;

};


pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t condc, condp; /*  Consumer/Producer condition variables */


queue *initialize_queue()
{
    struct int_queue *q;
    if ( (q = (struct int_queue *)malloc(sizeof(struct int_queue))) == NULL)
        exit(1);

    q->head= 0, 
    q->tail= MAX_QUEUE_SIZE-1;
    q->size = 0;
    return q;
}

/*  Using a simple circular queue array */
/*  Obtain a client from the waiting client queue*/
int pop_from_queue(queue *q) {
    int confd;
    printf("start pop\n");
    pthread_mutex_lock(&pool_mutex);

    while(q->size <= 0) {
        pthread_cond_wait(&condc, &pool_mutex);
        printf("woken up size is %ld\n",q->size);
    }

    printf("taking confd off queue\n");
    confd = q->items[q->head];
    q->head += 1;
    q->head %= MAX_QUEUE_SIZE;
    q->size--;

    pthread_cond_signal(&condp);
    pthread_mutex_unlock(&pool_mutex);

    return confd;
}


/*  Add a waiting client to the queue */
void push_to_queue(queue *q, int item) {
    
    pthread_mutex_lock(&pool_mutex);

    while(q->size >= MAX_QUEUE_SIZE) 
        pthread_cond_wait(&condp, &pool_mutex);
    
    q->tail += 1;
    q->tail %= MAX_QUEUE_SIZE;
    q->size ++;
    printf("adding confd to queue %ld, size now %ld\n",q->tail,q->size);

    q->items[q->tail] = item;

    pthread_cond_signal(&condc);
    pthread_mutex_unlock(&pool_mutex);
    printf("finished add\n");
}



