#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>
#include "syscall_number.h"

void sleep(uint32_t miliseconds);
uint32_t get_ticks();

#endif