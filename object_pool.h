#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
    // TODO: add other members to speed things up
    void *data;
} PoolElement;

typedef int (*Ctor)(void *(*malloc)(size_t), void (*free)(void *), void *dst, void *args);
typedef int (*Dtor)(void *(*malloc)(size_t), void (*free)(void *), void *dst);

typedef struct {
    void *(*malloc)(size_t size);
    void (*free)(void *mem);
    Ctor ctor;
    Dtor dtor;

    uint32_t size;
    uint32_t element_size;
    PoolElement *objects;
} ObjectPool;

ObjectPool op_allocate(uint32_t size, uint32_t element_size,
    void *(*)(size_t), void (*)(void *), Ctor ctor, Dtor dtor);
void op_release(ObjectPool *pool);
// although this function return index, you can access it through pool->objects[id]
int32_t op_register(ObjectPool *pool, void *args);
void op_unregister(ObjectPool *pool, int32_t id);

#endif