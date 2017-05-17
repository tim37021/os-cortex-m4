#ifndef _OS_H_
#define _OS_H_

#include <stdint.h>
#include "malloc.h"

#define MAX_TASKS 24
#define MAX_FIFO 10

#define LOW_PRIORITY 100
#define NORMAL_PRIORITY 1000
#define HIGH_PRIORITY 10000
#define AGING_VALUE 10
#define TASK_READY(t) ((t)->status==0 || ((t)->status<0 && ticks_counter >= -(t)->status))
#define TICKS_PER_SEC 10000

typedef int32_t pid_t;
typedef int32_t priority_t;
// 0 = ready
// <0 = -awake time, for example -10000, awake on ticks_counter = 10000
// 1 wait for IO
typedef int32_t status_t;

#define USB_DRIVER_PID 2

struct FIFOStruct;
typedef struct FIFOStruct FIFO;

typedef struct {
	pid_t pid;
	priority_t orig_priority;
	priority_t priority;
	status_t status;
	FIFO *mailbox;
	uint8_t *program_break;
	uint32_t *stack;
} tcb_t;

void os_init();
tcb_t os_create_task(uint32_t *space, void (*start)(), void *param, int stack_size, priority_t priority);
void os_start_schedule(tcb_t tasks[], int num_tasks);

#endif