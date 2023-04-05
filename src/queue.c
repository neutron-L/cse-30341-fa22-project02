/* queue.c: Concurrent Queue of Requests */

#include "mq/queue.h"

/**
 * Create queue structure.
 * @return  Newly allocated queue structure.
 */
Queue * queue_create() {
    Queue * ptr = (Queue *)malloc(sizeof(Queue));
    if (ptr)
    {
        ptr->head = ptr->tail = NULL;
        ptr->size = 0;
        // init the mutex lock and condition variable
        mutex_init(&ptr->lock, NULL);
        cond_init(&ptr->notempty, NULL);
    }

    return ptr;
}

/**
 * Delete queue structure.
 * @param   q       Queue structure.
 */
void queue_delete(Queue *q) {
    if (q)
    {
        Request * cur, * next;
        for (cur = q->head; cur; cur = next)
        {
            next = cur->next;
            request_delete(cur);
            cur = next;
        }

        // destroy the mutex lock and condition variable
        mutex_destroy(&q->lock);
        cond_destroy(&q->notempty);
    }

    free(q);
}

/**
 * Push request to the back of queue.
 * @param   q       Queue structure.
 * @param   r       Request structure.
 */
void queue_push(Queue *q, Request *r) {
    mutex_lock(&q->lock);  // lock
    
    if (q->tail)
        q->tail->next = r;
    q->tail = r;
    if (!q->head)
        q->head = r;
    ++q->size;

    cond_signal(&q->notempty);  // send 'notempty' signal 

    mutex_unlock(&q->lock);  // unlock
}

/**
 * Pop request to the front of queue (block until there is something to return).
 * @param   q       Queue structure.
 * @return  Request structure.
 */
Request * queue_pop(Queue *q) {
    Request * r;

    mutex_lock(&q->lock);  // lock

    while (q->size == 0)
        cond_wait(&q->notempty, &q->lock);
    
    // dequeue
    r = q->head;
    q->head = q->head->next;

    if (q->tail == r)
        q->tail = NULL;
    --q->size;

    mutex_unlock(&q->lock);  // unlock

    return r;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
