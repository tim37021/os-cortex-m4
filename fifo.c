#include "fifo.h"


FIFO fifo_init(void *data, uint32_t size)
{
    // we don't put things at rear
    return (FIFO) {.data=(uint8_t *)data, .size=size, .front=0, .rear=0};
}

static inline int32_t fifo_size(FIFO *fifo)
{
    return (fifo->rear>=fifo->front ? fifo->rear-fifo->front: fifo->size-fifo->front+fifo->rear);
}

int32_t fifo_write(FIFO *fifo, void *buffer, int32_t count)
{
    uint32_t remains = (fifo->size - 1) - fifo_size(fifo);
    if(count > remains) {
        count = remains;
    }
    if(fifo->front <= fifo->rear && fifo->rear + count >= fifo->size) {
        uint32_t len = fifo->size-fifo->rear;
        memcpy(fifo->data+fifo->rear, buffer, len);
        fifo->rear = (fifo->rear + count) % fifo->size;
        memcpy(fifo->data, (uint8_t *)buffer+len, count-len);
    } else {
        memcpy(fifo->data+fifo->rear, buffer, count);
        fifo->rear += count;
    }
    return count;
}

int32_t fifo_read(FIFO *fifo, void *buffer, int32_t count)
{
    uint32_t size = fifo_size(fifo);
    if(count > size)
        count = size;
    if(fifo->front > fifo->rear && fifo->front + count >= fifo->size) {
        uint32_t len = fifo->size-fifo->front;
        memcpy(buffer, fifo->data+fifo->front, len);
        fifo->front = (fifo->front + count) % fifo->size;
        memcpy((uint8_t *)buffer+len, fifo->data, count-len);
    } else {
        memcpy(buffer, fifo->data+fifo->front, count);
        fifo->front += count;
    }

    return count;
}