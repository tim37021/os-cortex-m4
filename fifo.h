#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdint.h>

typedef struct {
    uint8_t *data;
    uint32_t size;
    int32_t front;
    int32_t rear;
} FIFO;

FIFO fifo_init(void *data, uint32_t size);
int32_t fifo_write(FIFO *fifo, void *buffer, int32_t count);
int32_t fifo_read(FIFO *fifo, void *buffer, int32_t count);

#endif