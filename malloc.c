#include "syscall.h"
#include "stm32f4xx_it.h"
#include <stdio.h>
#include <stdint.h>

static inline size_t word_align(size_t size) {
    return (size + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1);
}

struct chunk {
    struct chunk *next, *prev;
    size_t        size;
    int           free;
    void         *data;
};

typedef struct chunk *Chunk;

#define KERNEL_HEAP 2048
void *ksbrk(int32_t inc)
{
    static uint8_t heap[KERNEL_HEAP];
    static int32_t kernel_break = 0;
    if(kernel_break + inc < 0 || kernel_break + inc > KERNEL_HEAP) {
        return (void *)-1;
    }
    int32_t old_break = kernel_break;
    kernel_break += inc;
    return &heap[old_break];
}


static void *malloc_base(void *(*_sbrk)(int32_t)) {
    static Chunk b = NULL;
    if (!b) {
        b = _sbrk(word_align(sizeof(struct chunk)));
        if (b == (void*) -1) {
            HardFault_Handler();
        }
        b->next = NULL;
        b->prev = NULL;
        b->size = 0;
        b->free = 0;
        b->data = NULL;
    }
    return b;
}

static Chunk malloc_chunk_find(void *(*_sbrk)(int32_t), size_t s, Chunk *heap) {
    Chunk c = malloc_base(_sbrk);
    for (; c && (!c->free || c->size < s); *heap = c, c = c->next);
    return c;
}

static void malloc_merge_next(Chunk c) {
    c->size = c->size + c->next->size + sizeof(struct chunk);
    c->next = c->next->next;
    if (c->next) {
        c->next->prev = c;
    }
}

static void malloc_split_next(Chunk c, size_t size) {
    Chunk newc = (Chunk)((char*) c + size);
    newc->prev = c;
    newc->next = c->next;
    newc->size = c->size - size;
    newc->free = 1;
    newc->data = newc + 1;
    if (c->next) {
        c->next->prev = newc;
    }
    c->next = newc;
    c->size = size - sizeof(struct chunk);
}

static void *_malloc(void *(*_sbrk)(int32_t), size_t size) {
    if (!size) return NULL;
    size_t length = word_align(size + sizeof(struct chunk));
    Chunk prev = NULL;
    Chunk c = malloc_chunk_find(_sbrk, size, &prev);
    if (!c) {
        Chunk newc = _sbrk(length);
        if (newc == (void*) -1) {
            return NULL;
        }
        newc->next = NULL;
        newc->prev = prev;
        newc->size = length - sizeof(struct chunk);
        newc->data = newc + 1;
        prev->next = newc;
        c = newc;
    } else if (length + sizeof(size_t) < c->size) {
        malloc_split_next(c, length);
    }
    c->free = 0;
    return c->data;
}

static void _free(void *(*_sbrk)(int32_t), void *ptr) {
    if (!ptr || ptr < malloc_base(_sbrk) || ptr > _sbrk(0)) return;
    Chunk c = (Chunk) ptr - 1;
    if (c->data != ptr) return;
    c->free = 1;

    if (c->next && c->next->free) {
        malloc_merge_next(c);
    }
    if (c->prev->free) {
        malloc_merge_next(c = c->prev);
    }
    if (!c->next) {
        c->prev->next = NULL;
        _sbrk(- c->size - sizeof(struct chunk));
    }
}

static void *_calloc(void *(*_sbrk)(int32_t), size_t nmemb, size_t size) {
    size_t length = nmemb * size;
    void *ptr = _malloc(_sbrk, length);
    if (ptr) {
        char *dst = ptr;
        for (size_t i = 0; i < length; *dst = 0, ++dst, ++i);
    }
    return ptr;
}

static void *_realloc(void *(*_sbrk)(int32_t), void *ptr, size_t size) {
    void *newptr = _malloc(_sbrk, size);
    if (newptr && ptr && ptr >= malloc_base(_sbrk) && ptr <= _sbrk(0)) {
        Chunk c = (Chunk) ptr - 1;
        if (c->data == ptr) {
            size_t length = c->size > size ? size : c->size;
            char *dst = newptr, *src = ptr;
            for (size_t i = 0; i < length; *dst = *src, ++src, ++dst, ++i);
            _free(_sbrk, ptr);
        }
    }
    return newptr;
}

void *malloc(size_t size) 
{
    return _malloc(sbrk, size);
}

void free(void *ptr) 
{
    _free(sbrk, ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    return _calloc(sbrk, nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return _realloc(sbrk, ptr, size);
}

void *kmalloc(size_t size) 
{
    return _malloc(ksbrk, size);
}

void kfree(void *ptr) 
{
    _free(ksbrk, ptr);
}