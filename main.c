#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"
#include "syscall.h"
#include "priority_queue.h"
#include "malloc.h"
#include "fifo.h"
#include <tm_stm32f4_usb_hid_device.h>

const char *key_name[4][4] = {
	{"1.1", "2.1", "3.1", "4.1"}, 
	{"1.2", "2.2", "3.2", "4.2"},
	{ "1.3", "2.3", "3.3", "4.3"},
	{ "1.4", "2.4", "3.4", "4.4"}
};

uint32_t idle_task_stack[36], usb_driver_task_stack[128], task_stack[64], task_stack2[64];
uint32_t main_task_stack[512];

#define FORCE_NEXT_PRIORITY 10000
#define NORMAL_PRIORITY 1000
#define LOW_PRIORITY 100
#define AGING_VALUE 10
#define TASK_READY(t) ((t)->status==0 || ((t)->status<0 && ticks_counter >= -(t)->status))

void syscall();
volatile uint32_t ticks_counter=0;

struct TCB {
	int pid;
	int orig_priority;
	int priority;
	// 0 = ready, 1 suspended, <0 = sleep milisecond
	int status;
	FIFO *mailbox;
	uint8_t *program_break;
	uint32_t *stack;
};

typedef struct {
	int from;
	int tag;
	uint32_t size;
} MailHeader;

#define MAX_TASK 5
struct TCB tasks[MAX_TASK];
struct TCB *tasks_queue[MAX_TASK+1];

#define MAX_FIFO 10

static void init(void)
{
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	TM_USB_HIDDEVICE_Init();
}

static void scan(IOInterface *interface, int last_result[][4], KeyEvent cur_event[][4])
{
	//LCD_DisplayStringLine(LCD_LINE_1, str);
	int result[4][4];
	scan_keybd(interface, 4, 4, result);
	update_keybd_event(4, 4, last_result, result, cur_event);
}

__attribute__((naked)) void idle_task()
{
	while(1) {
	}
}

void usb_driver_task()
{
	int32_t pid, tag;
	unsigned char msg[24];

	attach_fifo(register_fifo(128));
	while(1) {
		int32_t len=receive(&pid, &tag, msg, 24);
		if(TM_USB_HIDDEVICE_GetStatus() == TM_USB_HIDDEVICE_Status_Connected)
			TM_USB_HIDDEVICE_SendCustom(msg, len);
		else
			sleep(20);
	}
}

struct test_task_param {
	uint16_t pin;
	uint32_t delay;
};

void test_task(struct test_task_param *param_)
{
	struct test_task_param param = *param_;
	int on_off = 0;

	// Enable gpio clock
	init_output_pins(GPIOE, param.pin);
	GPIO_ResetBits(GPIOE, param.pin);

	char msg[12]=" LED_ON";
	msg[0] = get_pid()+'0';
	while(1) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off) {
			GPIO_SetBits(GPIOE, param.pin);
			send(2, 0, msg, strlen(msg));
		} else
			GPIO_ResetBits(GPIOE, param.pin);
		sleep(param.delay);
	}
}

void main_task() {
	int last_result[4][4];
	KeyEvent cur_event[4][4];
	char text[64];
	text[0] = '\0';
	IOInterface *interface = init_stm32_keybd();

	while(1) {
		scan(interface, last_result, cur_event);
		for(int i=0; i<4; i++) {
			for(int j=0; j<4; j++) {
				if(cur_event[i][j]==KEY_DOWN)
					strcat(text, key_name[i][j]);
			}
		}

		if(text[0]) {
			send(1, 0, text, strlen(text));
			text[0]='\0';
		}

		// sleep for 20 ms
		sleep(20);
	}
}

void *activate(void *);

struct TCB create_task(uint32_t *space, void (*start)(), void *param, int stack_size, int priority, int first) 
{
	static int next_pid = 1;
	uint32_t *stack = space;
	stack +=  stack_size - 32;
	if(first) {
		stack[8] = (uint32_t)start;
	} else {
		stack[8] = (uint32_t)0xFFFFFFFD;
		stack[15] = (uint32_t)start;
		stack[16] = (uint32_t)0x01000000;
	}
	// r0
	stack[9] = (uint32_t)param;
	return (struct TCB) {.pid = next_pid++, .status=0, .orig_priority=priority, 
		.priority=priority, .program_break=(uint8_t *)space, .mailbox=NULL, .stack=stack};
}

static int compare(const void *lhs, const void *rhs) {
	return ((const struct TCB *)lhs)->priority > ((const struct TCB *)rhs)->priority;
}

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

void clear_svc_flag()
{
	
}

#define TICKS_PER_SEC 10000
int main(void)
{
	SystemInit();
	SysTick_Config(SystemCoreClock / TICKS_PER_SEC); // SysTick event each 10ms

	init();

	PriorityQueue q = pq_init((const void **)tasks_queue, compare);

	int num_tasks=0;
	tasks[num_tasks++] = create_task(idle_task_stack, idle_task, NULL, 36, LOW_PRIORITY, 1);
	tasks[num_tasks++] = create_task(usb_driver_task_stack, usb_driver_task, NULL, 128, NORMAL_PRIORITY, 0);

	struct test_task_param param1={.pin=GPIO_Pin_8, .delay=1000}, param2={.pin=GPIO_Pin_10, .delay=100};
	tasks[num_tasks++] = create_task(task_stack, test_task, &param1, 64, NORMAL_PRIORITY,  0);
	tasks[num_tasks++] = create_task(task_stack2, test_task, &param2, 64, NORMAL_PRIORITY,  0);
	tasks[num_tasks++] = create_task(main_task_stack, main_task, NULL, 512, NORMAL_PRIORITY,  0);

	for(int i=0; i<num_tasks; i++) {
		pq_push(&q, &tasks[i]);
	}

	tasks[0].stack = activate(tasks[0].stack);

	ObjectPool fifo_pool = fp_init(MAX_FIFO, kmalloc, kfree);
	while (1) {
		struct TCB *top;
		void *stored_tasks[MAX_TASK];
		int num_stored_tasks=0;
		while((top = (struct TCB *)pq_top(&q)) && !TASK_READY(top)) {
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
				ticks_counter++; break;
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
					struct TCB *dst_task = &tasks[dst-1];
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

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
