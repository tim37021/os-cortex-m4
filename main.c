#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"

static const char *str="Hi, tim37021, ps2747";
static int on_off = 0;
IOInterface *interface;
int last_result[4][4];
KeyEvent cur_event[4][4];
char *key_name[4][4] = {"1.1", "2.1", "3.1", "4.1", "1.2", "2.2", "3.2", "4.2", "1.3", "2.3", "3.3", "4.3", "1.4", "2.4", "3.4", "4.4"};
int n;
char text[256];
uint32_t stack[512];

void syscall();

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

static void render(void)
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

void test_task()
{
	while(1) {
		update();
		render();
	}
}

void test_task2()
{
	while(1) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off)
			GPIO_SetBits(GPIOE,GPIO_Pin_8);
		else
			GPIO_ResetBits(GPIOE,GPIO_Pin_8);
		for(int i=0; i<100000; i++);
	}
}

uint32_t *create_task(uint32_t *stack, void (*start)()) 
{
	stack +=  512 - 32;
	stack[8] = (uint32_t)start;

	return stack;
}

uint32_t activate(void *);

////////////////////
void RCC_Configuration(void)
{
    /* --------------------------- System Clocks Configuration -----------------*/
    /* USART1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    /* GPIOA clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
}

void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*-------------------------- GPIO Configuration ----------------------------*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Connect USART pins to AF */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);   // USART1_TX
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // USART1_RX
}

void USART1_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;

    /* USARTx configuration ------------------------------------------------------*/
    /* USARTx configured as follow:
     *  - BaudRate = 9600 baud
     *  - Word Length = 8 Bits
     *  - One Stop Bit
     *  - No parity
     *  - Hardware flow control disabled (RTS and CTS signals)
     *  - Receive and transmit enabled
     */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);


    USART_Cmd(USART1, ENABLE);
}

/////////////////////

int main(void)
{

	init();

	uint32_t *task_stack[2];

	task_stack[0] = create_task(stack, test_task);
	//task_stack[1] = create_task(stack2, test_task2);

	SysTick_Config(SystemCoreClock / 1000); // SysTick event each 10ms

	while (1) {
		task_stack[0] = activate(task_stack[0]);
	}
}

void svc_handler(int value)
{

}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
