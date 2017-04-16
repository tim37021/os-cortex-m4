#include "stm32f4xx.h"
#include "arm_math.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"
#include "utility.h"
#include "keybd_stm32.h"
#include <string.h>
#include <stdio.h>

static const char *str="Hi, tim37021, ps2747";
static int on_off = 0;
IOInterface *interface;
int last_result[4][4];
KeyEvent cur_event[4][4];
char key_name[4][4] = {'1', '2', '3', 'A', '4', '5', '6', 'B', '7', '8', '9', 'C', '*', '0', '#', 'D'};
int n;
char text[256];

static void init(void)
{
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	interface = init_stm32_keybd();

	// Enable gpio clock
	init_output_pins(GPIOE, GPIO_Pin_8);
	GPIO_ResetBits(GPIOE, GPIO_Pin_8);
	
	init_input_pins(GPIOD, GPIO_Pin_12, GPIO_PuPd_DOWN);
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
			if(n<254&&cur_event[i][j]==KEY_DOWN)
				text[n++] = key_name[i][j];
		}
	}
	// DELAY
	for(int i=0; i<10000; i++);
}

int main(void)
{
	SysTick_Config(SystemCoreClock / 100); // SysTick event each 10ms
	init();

	while (1) {
		update();
		render();
	}
}

void OnSysTick(void)
{
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	/* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	while (1) { }
}
#endif
