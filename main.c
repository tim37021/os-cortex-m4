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
	init_output_pins(GPIOG, param.pin);
	GPIO_ResetBits(GPIOG, param.pin);

	while(1) {
		on_off = !on_off;
		if(on_off) {
			GPIO_SetBits(GPIOG, param.pin);
		} else
			GPIO_ResetBits(GPIOG, param.pin);
		sleep(param.delay);
	}
}

#define RESET_CMD 0xAB
#define BEGIN_CMD 0xCD
#define END_CMD 0xEF

void main_task() {
	int result[4][4];
	IOInterface *interface = init_stm32_keybd();

	uint32_t ticks = get_ticks();

	uint32_t enabled = 0;
	uint8_t buf[64];
	uint8_t cmd;
	int buf_size=0;
	while(1) {
		if(read_fifo_nb(STDIN, &cmd, 1) == 1) {
			switch(cmd) {
				case RESET_CMD:
					ticks = get_ticks(); break;
				case BEGIN_CMD:
					enabled = 1; break;
				case END_CMD:
					enabled = 0; break;
			}
		}

		scan_keybd(interface, 4, 4, result);

		if(enabled) {
			*(uint32_t *)buf = get_ticks() - ticks;
			buf_size = 5;
			for(int i=0; i<4; i++) {
				for(int j=0; j<4; j++) {
					if(result[i][j] == 1) {
						buf[buf_size++] = j*7+i;
					}
				}
			}
			buf[4] = buf_size - 5;
			buf[buf_size++] = 0xFF;
			// ending
			send(USART_DRIVER_PID, 0, buf, buf_size);
		}

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
	struct test_task_param param1={.pin=GPIO_Pin_13, .delay=1000}, param2={.pin=GPIO_Pin_14, .delay=100};
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
