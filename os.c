#include "os.h"
#include "priority_queue.h"
#include "object_pool.h"
#include "syscall.h"
#include "stm32f4xx.h"
#include "fifo.h"

volatile uint32_t ticks_counter=0;
static tcb_t *tasks_queue[MAX_TASKS];
static ObjectPool fifo_pool;
static PriorityQueue q;
static tcb_t tasks[MAX_TASKS];
static int num_kernel_tasks = 0;
static pid_t next_pid = 2;

typedef struct {
	int from;
	int tag;
	uint32_t size;
} MailHeader;

void *activate(void *);

static int compare(const void *lhs, const void *rhs) {
	return ((const tcb_t *)lhs)->priority > ((const tcb_t *)rhs)->priority;
}

//////////////////////////////////////////////////
__attribute__((naked)) static void idle_task()
{
	while(1) {
	}
}

#define IDLE_TASK_SPACE_SIZE 36
#define IDLE_TASK_STACK_START (IDLE_TASK_SPACE_SIZE-17)
static uint32_t idle_task_space[IDLE_TASK_SPACE_SIZE] = {[IDLE_TASK_STACK_START+8]=(uint32_t)idle_task};
static tcb_t idle_task_tcb = {.pid = 1, .status=0, .orig_priority=LOW_PRIORITY, 
		.priority=LOW_PRIORITY, .program_break=(uint8_t *)idle_task_space, .mailbox=NULL, .stack=&idle_task_space[IDLE_TASK_STACK_START]};
///////////////////////////////////////////////////
#ifdef USE_USB_DRIVER
#include <tm_stm32f4_usb_hid_device.h>
static void usb_driver()
{
	int32_t pid, tag;
	unsigned char msg[24];

	attach_fifo(register_fifo(128));
	while(1) {
		int32_t len=receive(&pid, &tag, msg, 24);
		if(TM_USB_HIDDEVICE_GetStatus() == TM_USB_HIDDEVICE_Status_Connected){
			//TM_USB_HIDDEVICE_SendCustom(msg, len);
		} else
			sleep(20);
	}
}

#define USB_DRIVER_SPACE 128
static uint32_t usb_driver_space[usb_DRIVER_SPACE];
#endif
//////////////////////////////////////////////////////
static void usart1_init()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// Initialize pins as alternating function
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_InitStruct.USART_BaudRate = 9600;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(USART1_IRQn);
}

static void usart1_puts(const uint8_t *s, int len)
{
    for(int i=0; i<len; i++) {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, s[i]);
    }
}

void USART1_IRQHandler()
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){         
        char c = USART_ReceiveData(USART1);
		fifo_write((FIFO *)fifo_pool.objects[STDIN].data, &c, 1);
	}
}

static void usart_driver()
{
	int32_t pid, tag;
	unsigned char msg[24];

	attach_fifo(register_fifo(128));
	while(1) {
		int32_t len=receive(&pid, &tag, msg, 24);
		usart1_puts(msg, len);
	}
}

#define USART_DRIVER_SPACE 128
static uint32_t usart_driver_space[USART_DRIVER_SPACE];
//////////////////////////////////////////////////////

// true: has mail or error
static int receive_mailbox(int pid) 
{
	// pid start from one...
	pid-=1;
	void *param1 = &tasks[pid].stack[9];
	void *param2 = &tasks[pid].stack[10];
	void *param3 = &tasks[pid].stack[11];
	void *param4 = &tasks[pid].stack[12];

	// if this process has no mailbox, return error
	// return 1...
	if(tasks[pid].mailbox==NULL) {
		*(int32_t *)param1 = -1;
		return 1;
	}
	MailHeader header;
	int len = fifo_read(tasks[pid].mailbox, &header, sizeof(header));
	if(len != 0) {
		**(int32_t **)param1 = header.from;
		**(int32_t **)param2 = header.tag;
		*(int32_t *)param1 = fifo_read(tasks[pid].mailbox, *(void **)param3, 
			*(uint32_t *)param4<=header.size?*(uint32_t *)param4:header.size);
		// skip aging technique
		return 1;
	} 
	return 0;
}

void os_init()
{
	//NVIC_SetPriority(SysTick_IRQn, 0);
	//NVIC_SetPriority(SVCall_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
	//NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1);

	// init usb things..

	next_pid = 2;
	fifo_pool = fp_init(MAX_FIFO, kmalloc, kfree);
	FIFOCreateInfo info = {.size = 256};
	// THIS MUST BE 0 for input
	op_register(&fifo_pool, &info);

	q = pq_init((const void **)tasks_queue, compare);
	// create driver task
	int num_tasks = 0;
	tasks[num_tasks++] = idle_task_tcb;
	tasks[num_tasks++] = os_create_task(usart_driver_space, usart_driver, NULL, USART_DRIVER_SPACE, NORMAL_PRIORITY);
	usart1_init();
#ifdef USB_USB_DRIVER
	tasks[num_tasks++] = os_create_task(usb_driver_space, usb_driver, NULL, USB_DRIVER_SPACE, NORMAL_PRIORITY);
	TM_USB_HIDDEVICE_Init();
#endif
	// push n fundamental task
	num_kernel_tasks = num_tasks;

	SysTick_Config(SystemCoreClock / TICKS_PER_SEC); // SysTick event each 10ms
	
	
}

tcb_t os_create_task(uint32_t *space, void (*start)(), void *param, int stack_size, priority_t priority)
{
	uint32_t *stack = space;
	stack +=  stack_size - 17;

	stack[8] = (uint32_t)0xFFFFFFFD;
	stack[15] = (uint32_t)start;
	stack[16] = (uint32_t)0x01000000;

	// r0
	stack[9] = (uint32_t)param;
	return (tcb_t) {.pid = next_pid++, .status=0, .orig_priority=priority, 
		.priority=priority, .program_break=(uint8_t *)space, .mailbox=NULL, .stack=stack};
}

void os_start_schedule(tcb_t user_tasks[], int num_tasks)
{
	for(int i=0; i<num_tasks; i++) {
		tasks[num_kernel_tasks+i] = user_tasks[i];
	}
	for(int i=0; i<num_kernel_tasks+num_tasks; i++) {
		pq_push(&q, &tasks[i]);
	}

	// run idle_tasks
	tasks[0].stack = activate(tasks[0].stack);

	void *stored_tasks[MAX_TASKS];
	while (1) {
		tcb_t *top;
		int num_stored_tasks=0;
		
		while((top = (tcb_t *)pq_top(&q)) && !TASK_READY(top)) {
			stored_tasks[num_stored_tasks++] = top;
			pq_pop(&q);
		}
		// push back stored_tasks
		for(int i=0; i<num_stored_tasks; i++) {
			pq_push(&q, stored_tasks[i]);
		}
		// now top is the one we are going to activate
		top->status = 0;
		top->priority = top->orig_priority;
		top->stack = activate(top->stack);
		int func_number = top->stack[-1];

		// param1 is also return value r0, r1, r2, r3
		void *param1 = &top->stack[9];
		void *param2 = &top->stack[10];
		void *param3 = &top->stack[11];
		void *param4 = &top->stack[12];
		switch(func_number) {
			case 0:
				break;
			case GET_PID_SVC_NUMBER:
				*(uint32_t *)param1 = top->pid;
				continue;
			case SLEEP_SVC_NUMBER: // SLEEP
				top->status = -(TICKS_PER_SEC*(*(int32_t *)param1)/1000 + (int32_t)ticks_counter);
				break;
			case GET_TICKS_SVC_NUMBER: // CLOCK
				*(uint32_t *)param1 = ticks_counter;
				// skip aging technique
				continue;
			case SBRK_SVC_NUMBER: // sbrk
				// TODO: Check lower bound
				if((uint32_t)top->stack > (uint32_t)(top->program_break+*(int32_t *)param1)) {
					uint32_t prev = (uint32_t)top->program_break;
					top->program_break+=*(int32_t *)param1;
					*(int32_t *)param1 = prev;
				} else
					*(int32_t *)param1 = -1;
				// skip aging technique
				continue;
			case REGISTER_FIFO_SVC_NUMBER:
				{
					FIFOCreateInfo info = {.size = *(int32_t *)param1};
					*(int32_t *)param1 = op_register(&fifo_pool, &info);
				}
				continue;
			case UNREGISTER_FIFO_SVC_NUMBER:
				op_unregister(&fifo_pool, *(int32_t *)param1);
				continue;
			case ATTACH_FIFO_SVC_NUMBER:
				// TODO: Check the parameter
				top->mailbox = (FIFO *)fifo_pool.objects[*(int32_t *)param1].data;
				if(fifo_free_space(top->mailbox) < sizeof(MailHeader)) {
					*(int32_t *)param1 = -1;
					top->mailbox = NULL;
				}
				continue;
			case SEND_SVC_NUMBER:
				{
					MailHeader header = {.from=top->pid, .tag=*(int32_t *)param2, .size=*(int32_t *)param4};
					int32_t dst = *(int32_t *)param1;
					tcb_t *dst_task = &tasks[dst-1];
					if(dst_task->mailbox) {
						// READY!!
						if(fifo_free_space(dst_task->mailbox) >= sizeof(MailHeader)) {
							fifo_write(dst_task->mailbox, &header, sizeof(MailHeader));
							*(int32_t *)param1 = fifo_write(dst_task->mailbox, *(void **)param3, header.size);
						}
						// Process is waiting on mailbox, ugly hack.....
						if(dst_task->status==1) {
							receive_mailbox(dst);
							dst_task->status = 0;
						}
						continue;
					} else {
						*(int32_t *)param1 = -1;
						continue;
					}
				}
			case RECEIVE_SVC_NUMBER:
				if(receive_mailbox(top->pid))
					continue;
				else
					top->status = 1;
				// len == 0 blocks the call
				// if len not zero and not sizeof(header) something went so wrong
				break;
			case WRITE_FIFO_SVC_NUMBER:
				// TODO
				break;
			case READ_FIFO_SVC_NUMBER:
				*(int32_t *)param1 = fifo_read((FIFO *)fifo_pool.objects[*(int32_t *)param1].data, *(void **)param2, *(int32_t *)param3);
				continue;
		} 
		// no need to push back top

		// aging technique
		for(int i=1; i<=num_tasks; i++) {
			if(tasks_queue[i] != top) {
				tasks_queue[i]->priority += AGING_VALUE;
			}
		}
		pq_refresh(&q);
	}
}