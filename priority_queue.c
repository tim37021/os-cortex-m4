#include "priority_queue.h"


PriorityQueue pq_init(void **arr, int (*func)(void *, void *))
{
    return (PriorityQueue){.data=arr, .n=0, .compare=func};
}

static void heapify(PriorityQueue *q, int i)
{
    while(i>1 && q->compare(q->data[i/2], q->data[i])) {
        // swap..
        void *tmp = q->data[i/2];
        q->data[i/2] = q->data[i];
        q->data[i] = tmp;
        i/=2;
    }
}

void pq_push(PriorityQueue *q, void *data)
{
    q->n++;
    q->data[q->n] = data;
    heapify(q, q->n);
}

void *pq_top(PriorityQueue *q)
{
    return q->data[1];
}

void pq_pop(PriorityQueue *q)
{
    // swap..
    void *tmp = q->data[q->n];
    q->data[q->n] = q->data[1];
    q->data[1] = tmp;

    q->n--;
    heapify(q, q->n);
}