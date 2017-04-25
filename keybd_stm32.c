#include "keybd.h"
#include "utility.h"
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"

typedef struct Pin {
	GPIO_TypeDef *gpio_id;
	uint16_t pin_index;
} Pin;


static void stm32_write_row(int index, int value);
static int stm32_read_col(int index);
static IOInterface interface = {.write_row=stm32_write_row, .read_col=stm32_read_col};

static Pin row_pins[4] = {{.gpio_id=GPIOC, .pin_index=GPIO_Pin_10}, {.gpio_id=GPIOC, .pin_index=GPIO_Pin_12}, {.gpio_id=GPIOD, .pin_index=GPIO_Pin_1}, {.gpio_id=GPIOD, .pin_index=GPIO_Pin_3}};
static Pin col_pins[4] = {{.gpio_id=GPIOD, .pin_index=GPIO_Pin_5}, {.gpio_id=GPIOD, .pin_index=GPIO_Pin_7}, {.gpio_id=GPIOG, .pin_index=GPIO_Pin_9}, {.gpio_id=GPIOG, .pin_index=GPIO_Pin_11}};

IOInterface *init_stm32_keybd()
{
	for(int i=0; i<4; i++) {
		init_output_pins(row_pins[i].gpio_id, row_pins[i].pin_index);
		init_input_pins(col_pins[i].gpio_id, col_pins[i].pin_index, GPIO_PuPd_UP);
	}
 
    init_keybd(&interface, 4, 4);
    return &interface;
}

static void stm32_write_row(int index, int value)
{
	if(value==0) {
		init_output_pins(row_pins[index].gpio_id, row_pins[index].pin_index);
		GPIO_ResetBits(row_pins[index].gpio_id,row_pins[index].pin_index);
	} else {
		// when HIGH set to input mode
		init_input_pins(row_pins[index].gpio_id, row_pins[index].pin_index, GPIO_PuPd_UP);
		//for(int i=0; i<1000;i++);
		GPIO_SetBits(row_pins[index].gpio_id,row_pins[index].pin_index);
	}
}

static int stm32_read_col(int index)
{
	return GPIO_ReadInputDataBit(col_pins[index].gpio_id, col_pins[index].pin_index);
}