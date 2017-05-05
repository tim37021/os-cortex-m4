#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>
#include "syscall_number.h"

void sleep(uint32_t miliseconds);
uint32_t get_ticks();
void *sbrk(int32_t inc);
int32_t register_fifo(uint32_t size);
int32_t unregister_fifo(int32_t id);
// attach fifo to process
int32_t attach_fifo(int32_t id);
int32_t send(int32_t pid, int32_t tag, void *buf, int32_t count);
int32_t receive(int32_t *pid, int32_t *tag, void *buf, int32_t count);

#endif