#include "priority_queue.h"


PriorityQueue pq_init(const void **arr, int (*func)(const void *, const void *))
{
    return (PriorityQueue){.data=arr, .n=0, .compare=func};
}

static void heapify(PriorityQueue *q, int i)
{
    while(i*2<=q->n) {
        void **left = &q->data[i*2];
        void **right = &q->data[i*2+1];
        void **winner = (i*2+1>q->n)||q->compare(*left, *right)? left: right;
        if(q->compare(*winner, q->data[i])) {
            void *tmp = q->data[i];
            q->data[i] = *winner;
            *winner = tmp; 
        } else
            break;
        if(winner == left)
            i=i*2;
        else
            i=i*2+1;
    }
}

static void heapify_upward(PriorityQueue *q, int i)
{
    while(i>=1) {
        void **left = &q->data[i*2];
        void **right = &q->data[i*2+1];
        void **winner = (i*2+1>q->n)||q->compare(*left, *right)? left: right;
        if(q->compare(*winner, q->data[i])) {
            void *tmp = q->data[i];
            q->data[i] = *winner;
            *winner = tmp; 
        } else
            break;
        i/=2;
    }
}

void pq_push(PriorityQueue *q, const void *data)
{
    q->n++;
    q->data[q->n] = data;
    if(q->n>1) {
        heapify_upward(q, q->n/2);
    }
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
    heapify(q, 1);
}

void pq_refresh(PriorityQueue *q) 
{
    for(int i=q->n/2; i>=1; i--) {
        heapify(q, i);
    }
}
