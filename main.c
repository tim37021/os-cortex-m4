#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"
#include "syscall.h"
#include "malloc.h"
#include "fifo.h"
#include "os.h"

const char *key_name[4][4] = {
	{"1.1", "2.1", "3.1", "4.1"}, 
	{"1.2", "2.2", "3.2", "4.2"},
	{ "1.3", "2.3", "3.3", "4.3"},
	{ "1.4", "2.4", "3.4", "4.4"}
};

uint32_t task_stack[64], task_stack2[64];
uint32_t main_task_stack[512];

static void init(void)
{
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
}

static void scan(IOInterface *interface, int last_result[][4], KeyEvent cur_event[][4])
{
	//LCD_DisplayStringLine(LCD_LINE_1, str);
	int result[4][4];
	scan_keybd(interface, 4, 4, result);
	update_keybd_event(4, 4, last_result, result, cur_event);
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

	char msg[12]=" LED_ON\r\n";
	msg[0] = get_pid()+'0';
	while(1) {
		//LCD_Clear(0xFFFF);
		on_off = !on_off;
		if(on_off) {
			GPIO_SetBits(GPIOE, param.pin);
			send(USART_DRIVER_PID, 0, msg, strlen(msg));
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
			send(USART_DRIVER_PID, 0, text, strlen(text));
			text[0]='\0';
		}

		// sleep for 20 ms
		sleep(20);
	}
}

int main(void)
{
	SystemInit();

	//init();
	os_init();

	int num_tasks = 0;
	tcb_t tasks[5];
	struct test_task_param param1={.pin=GPIO_Pin_8, .delay=1000}, param2={.pin=GPIO_Pin_10, .delay=100};
	tasks[num_tasks++] = os_create_task(task_stack, test_task, &param1, 64, NORMAL_PRIORITY);
	tasks[num_tasks++] = os_create_task(task_stack2, test_task, &param2, 64, NORMAL_PRIORITY);
	tasks[num_tasks++] = os_create_task(main_task_stack, main_task, NULL, 512, NORMAL_PRIORITY);

	os_start_schedule(tasks, num_tasks);
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
