#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"
#include "syscall.h"
#include "priority_queue.h"

static const char *str="Hi, tim37021, ps2747";
static int on_off = 0;
IOInterface *interface;
int last_result[4][4];
KeyEvent cur_event[4][4];
char *key_name[4][4] = {"1.1", "2.1", "3.1", "4.1", "1.2", "2.2", "3.2", "4.2", "1.3", "2.3", "3.3", "4.3", "1.4", "2.4", "3.4", "4.4"};
int n;
char text[256];
uint32_t stack[512], stack2[512];
int cur_task = 0;

void syscall();
volatile uint32_t ticks_counter=0;

struct TCB {
	int priority;
	// 0 = ready, 1 suspended, <0 = sleep milisecond
	int status;
	uint32_t *stack;
};

#define MAX_TASK 5
struct TCB tasks[MAX_TASK];
void *tasks_queue[MAX_TASK+1];
int num_tasks=0;

static void init_usart1()
{
    /******** 宣告 USART、GPIO 結構體 ********/
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    /******** 啟用 GPIOA、USART1 的 RCC 時鐘 ********/
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

    /******** USART 基本參數設定 ********/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_InitStruct.USART_BaudRate = 9600;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);
}

void USART1_puts(char* s)
{
    while(*s) {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *s);
        s++;
    }
}

static void init(void)
{
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	interface = init_stm32_keybd();

	// Enable gpio clock
	init_output_pins(GPIOE, GPIO_Pin_8);
	GPIO_ResetBits(GPIOE, GPIO_Pin_8);
	
	init_input_pins(GPIOD, GPIO_Pin_12, GPIO_PuPd_DOWN);

	init_usart1();
}

static void update(void)
{

	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12)) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off)
			GPIO_SetBits(GPIOE,GPIO_Pin_8);
		else
			GPIO_ResetBits(GPIOE,GPIO_Pin_8);
		// something here
		str = "and, bobo";
		for(int i=0; i<100000; i++);
		while (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12)); // debounce
	}


	if(STM_EVAL_PBGetState(BUTTON_USER)) {
		for(int i=0; i<100000; i++);
		while (STM_EVAL_PBGetState(BUTTON_USER)); // debounce
	}
	
}

static void scan(void)
{
	//LCD_DisplayStringLine(LCD_LINE_1, str);
	int result[4][4];
	scan_keybd(interface, 4, 4, result);
	update_keybd_event(4, 4, last_result, result, cur_event);
	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			if(cur_event[i][j]==KEY_DOWN)
				strcat(text, key_name[i][j]);
		}
	}
	if(text[0]) {
		USART1_puts(text);
		text[0]='\0';
	}
}

void idle_task()
{
	while(1) {
		//sleep(1000);
	}
}

void test_task()
{
	while(1) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off)
			GPIO_SetBits(GPIOE,GPIO_Pin_8);
		else
			GPIO_ResetBits(GPIOE,GPIO_Pin_8);
		sleep(1000);
	}
}

struct TCB create_task(uint32_t *stack, void (*start)(), int priority, int first) 
{
	stack +=  512 - 32;
	if(first) {
		stack[8] = (uint32_t)start;
	} else {
		stack[8] = (uint32_t)0xFFFFFFFD;
		stack[15] = (uint32_t)start;
		stack[16] = (uint32_t)0x01000000;
	}
	return (struct TCB) {.status=0, .priority=priority, .stack=stack};
}

static int compare(const void *lhs, const void *rhs) {
	return ((const struct TCB *)lhs)->priority < ((const struct TCB *)rhs)->priority;
}

void *activate(void *);

#define TICKS_PER_SEC 10000
int main(void)
{
	init();


	tasks[num_tasks++] = create_task(stack, idle_task, 100, 1);
	tasks[num_tasks++] = create_task(stack2, test_task, 1000,  0);

	PriorityQueue q = pq_init(tasks_queue, compare);
	for(int i=0; i<num_tasks; i++) {
		pq_push(&q, &tasks[i]);
	}
	

	SysTick_Config(SystemCoreClock / TICKS_PER_SEC); // SysTick event each 10ms

	// activate the first task
	tasks[0].stack = activate(tasks[0].stack);
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
		top->stack = activate(top->stack);
		int func_number = top->stack[-1];
		void *param1 = &top->stack[9];
		switch(func_number) {
			case 0:
				ticks_counter++; break;
			case SLEEP_SVC_NUMBER: // SLEEP
				top->status = -(TICKS_PER_SEC*(*(int32_t *)param1)/1000 + (int32_t)ticks_counter);
				break;
		} 
		// no need to push back top
	}
}

__attribute__((naked)) void OnSysTick(void)
{
	ticks_counter++;
	__asm__("bx lr\n");
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
