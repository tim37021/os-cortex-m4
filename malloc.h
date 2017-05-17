#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif