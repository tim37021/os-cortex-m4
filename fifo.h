#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdint.h>
#include "object_pool.h"

typedef struct FIFOStruct {
    uint8_t *data;
    uint32_t size;
    int32_t front;
    int32_t rear;
} FIFO;

typedef struct {
    uint32_t size;
} FIFOCreateInfo;

// Helper function for FIFO Pool
ObjectPool fp_init(uint32_t size, void *(*malloc)(size_t), void (*free)(void *));
FIFO fifo_init(void *data, uint32_t size);
int32_t fifo_write(FIFO *fifo, void *buffer, int32_t count);
int32_t fifo_read(FIFO *fifo, void *buffer, int32_t count);
uint32_t fifo_free_space(FIFO *fifo);


#endif