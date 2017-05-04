#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"
#include "syscall.h"
#include "priority_queue.h"
#include "malloc.h"
#include <tm_stm32f4_usb_hid_device.h>

const char *key_name[4][4] = {
	{"1.1", "2.1", "3.1", "4.1"}, 
	{"1.2", "2.2", "3.2", "4.2"},
	{ "1.3", "2.3", "3.3", "4.3"},
	{ "1.4", "2.4", "3.4", "4.4"}
};

uint32_t idle_task_stack[36], task_stack[64], task_stack2[64];
uint32_t main_task_stack[512];

#define FORCE_NEXT_PRIORITY 10000
#define NORMAL_PRIORITY 1000
#define LOW_PRIORITY 100
#define AGING_VALUE 10

void syscall();
volatile uint32_t ticks_counter=0;

struct TCB {
	int pid;
	int orig_priority;
	int priority;
	// 0 = ready, 1 suspended, <0 = sleep milisecond
	int status;
	uint8_t *program_break;
	uint32_t *stack;
};

#define MAX_TASK 5
struct TCB tasks[MAX_TASK];
struct TCB *tasks_queue[MAX_TASK+1];

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

	while(1) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off)
			GPIO_SetBits(GPIOE, param.pin);
		else
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

	while(TM_USB_HIDDEVICE_GetStatus() != TM_USB_HIDDEVICE_Status_Connected);

	while(1) {
		scan(interface, last_result, cur_event);
		for(int i=0; i<4; i++) {
			for(int j=0; j<4; j++) {
				if(cur_event[i][j]==KEY_DOWN)
					strcat(text, key_name[i][j]);
			}
		}

		if(text[0]) {
			TM_USB_HIDDEVICE_SendCustom((uint8_t *)text, strlen(text));
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
	stack = activate(stack);
	return (struct TCB) {.pid = next_pid++, .status=0, .orig_priority=priority, 
		.priority=priority, .program_break=(uint8_t *)space, .stack=stack};
}

static int compare(const void *lhs, const void *rhs) {
	return ((const struct TCB *)lhs)->priority > ((const struct TCB *)rhs)->priority;
}

#define TICKS_PER_SEC 10000
int main(void)
{
	SystemInit();
	init();

	SysTick_Config(SystemCoreClock / TICKS_PER_SEC); // SysTick event each 10ms

	PriorityQueue q = pq_init((const void **)tasks_queue, compare);

	int num_tasks=0;
	tasks[num_tasks++] = create_task(idle_task_stack, idle_task, NULL, 36, LOW_PRIORITY, 1);

	struct test_task_param param1={.pin=GPIO_Pin_8, .delay=1000}, param2={.pin=GPIO_Pin_10, .delay=500};
	tasks[num_tasks++] = create_task(task_stack, test_task, &param1, 64, NORMAL_PRIORITY,  0);
	tasks[num_tasks++] = create_task(task_stack2, test_task, &param2, 64, NORMAL_PRIORITY,  0);
	tasks[num_tasks++] = create_task(main_task_stack, main_task, NULL, 512, NORMAL_PRIORITY,  0);

	for(int i=0; i<num_tasks; i++) {
		pq_push(&q, &tasks[i]);
	}


	while (1) {
		struct TCB *top;
		void *stored_tasks[MAX_TASK];
		int num_stored_tasks=0;
		while((top = (struct TCB *)pq_top(&q)) && (top->status!=0 && ticks_counter<-top->status)) {
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

		// param1 is also return value
		void *param1 = &top->stack[9];
		switch(func_number) {
			case 0:
				ticks_counter++; break;
			case SLEEP_SVC_NUMBER: // SLEEP
				top->status = -(TICKS_PER_SEC*(*(int32_t *)param1)/1000 + (int32_t)ticks_counter);
				break;
			case GET_TICKS_SVC_NUMBER: // CLOCK
				*(uint32_t *)param1 = ticks_counter;
				// skip aging technique
				continue;
			case SBRK_SVC_NUMBER: // sbrk
				if(top->stack > top->program_break+*(int32_t *)param1) {
					uint32_t prev = (uint32_t)top->program_break;
					top->program_break+=*(int32_t *)param1;
					*(int32_t *)param1 = prev;
				} else
					*(int32_t *)param1 = -1;
				// skip aging technique
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

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
