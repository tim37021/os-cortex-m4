#include "syscall.h"
#include "stm32f4xx_it.h"
#include <stdio.h>
#include <stdint.h>

static inline size_t word_align(size_t size) {
    return size + (sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

struct chunk {
    struct chunk *next, *prev;
    size_t        size;
    int           free;
    void         *data;
};

typedef struct chunk *Chunk;

static void *malloc_base() {
    static Chunk b = NULL;
    if (!b) {
        b = ksbrk(word_align(sizeof(struct chunk)));
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

static Chunk malloc_chunk_find(size_t s, Chunk *heap) {
    Chunk c = malloc_base();
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

void *malloc(size_t size) {
    if (!size) return NULL;
    size_t length = word_align(size + sizeof(struct chunk));
    Chunk prev = NULL;
    Chunk c = malloc_chunk_find(size, &prev);
    if (!c) {
        Chunk newc = ksbrk(length);
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

void free(void *ptr) {
    if (!ptr || ptr < malloc_base() || ptr > ksbrk(0)) return;
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
        ksbrk(- c->size - sizeof(struct chunk));
    }
}

void *calloc(size_t nmemb, size_t size) {
    size_t length = nmemb * size;
    void *ptr = malloc(length);
    if (ptr) {
        char *dst = ptr;
        for (size_t i = 0; i < length; *dst = 0, ++dst, ++i);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    void *newptr = malloc(size);
    if (newptr && ptr && ptr >= malloc_base() && ptr <= ksbrk(0)) {
        Chunk c = (Chunk) ptr - 1;
        if (c->data == ptr) {
            size_t length = c->size > size ? size : c->size;
            char *dst = newptr, *src = ptr;
            for (size_t i = 0; i < length; *dst = *src, ++src, ++dst, ++i);
            free(ptr);
        }
    }
    return newptr;
}

///////////////////////////////////////////////

#define KERNEL_HEAP 2048
void *fake_sbrk(int32_t inc)
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

static void *kmalloc_base() {
    static Chunk b = NULL;
    if (!b) {
        b = fake_sbrk(word_align(sizeof(struct chunk)));
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

void *kmalloc(size_t size) {
    if (!size) return NULL;
    size_t length = word_align(size + sizeof(struct chunk));
    Chunk prev = NULL;
    Chunk c = malloc_chunk_find(size, &prev);
    if (!c) {
        Chunk newc = fake_sbrk(length);
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

void kfree(void *ptr) {
    if (!ptr || ptr < malloc_base() || ptr > fake_sbrk(0)) return;
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
        fake_sbrk(- c->size - sizeof(struct chunk));
    }
}
