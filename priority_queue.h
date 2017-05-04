#ifndef _PRIORITY_QUEUE_H_
#define _PRIORITY_QUEUE_H_

typedef struct priority_queue {
    void **data;
    int n;
    int (*compare)(void *lhs, void *rhs);
} PriorityQueue;

PriorityQueue pq_init(const void **arr, int (*)(const void *, const void *));
void pq_push(PriorityQueue *q, const void *data);
void *pq_top(PriorityQueue *q);
void pq_pop(PriorityQueue *q);
void pq_refresh(PriorityQueue *q);

#endif
