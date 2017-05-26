#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>
#include "syscall_number.h"

int32_t get_pid();
void sleep(uint32_t miliseconds);
uint32_t get_ticks();
void *sbrk(int32_t inc);
int32_t register_fifo(uint32_t size);
int32_t unregister_fifo(int32_t id);
// attach fifo to process
int32_t attach_fifo(int32_t id);
int32_t send(int32_t pid, int32_t tag, void *buf, uint32_t count);
int32_t receive(int32_t *pid, int32_t *tag, void *buf, uint32_t count);
int32_t write_fifo(int32_t id, void *buf, uint32_t count);
int32_t read_fifo(int32_t id, void *buf, uint32_t count);
int32_t read_fifo_nb(int32_t id, void *buf, uint32_t count);

#endif