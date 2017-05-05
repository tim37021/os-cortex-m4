#include "object_pool.h"

ObjectPool op_allocate(uint32_t size, uint32_t element_size,
    void *(*malloc_)(size_t), void (*free_)(void *), Ctor ctor, Dtor dtor)
{
    ObjectPool ret;
    ret.malloc = malloc_;
    ret.free = free_;
    ret.ctor = ctor;
    ret.dtor = dtor;
    ret.element_size = element_size;
    ret.size = size;
    ret.objects = ret.malloc(sizeof(PoolElement) * size);
    for(uint32_t i=0; i<size; i++) {
        ret.objects[i].data = NULL;
    }
    return ret;
}
void op_release(ObjectPool *pool)
{
    for(uint32_t i=0; i<pool->size; i++) {
        pool->free(pool->objects[i].data);
    }
    pool->free(pool->objects);
}

int32_t op_register(ObjectPool *pool, void *args)
{
    uint32_t i;
    for(i=0; i<pool->size; i++) {
        if(pool->objects[i].data) {
            pool->objects[i].data = pool->malloc(pool->element_size);
            pool->ctor(pool->malloc, pool->free, pool->objects[i].data, args);
        }
    }
    return i<pool->size? (int32_t)i: -1;
}

void op_unregister(ObjectPool *pool, int32_t id)
{
    if(pool->objects[id].data) {
        pool->dtor(pool->malloc, pool->free, pool->objects[id].data);
        pool->objects[id].data = NULL;
    }
}
