#include "utility.h"
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"

void init_output_pins(GPIO_TypeDef *gpio_id, uint16_t pins)
{
	uint32_t offset = ((uint32_t)gpio_id-(uint32_t)GPIOA)>>10;
	RCC_AHB1PeriphClockCmd((RCC_AHB1Periph_GPIOA<<offset), ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = pins;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
    GPIO_Init(gpio_id, &GPIO_InitStructure);
}

void init_input_pins(GPIO_TypeDef *gpio_id, uint16_t pins, GPIOPuPd_TypeDef pupd)
{
	uint32_t offset = ((uint32_t)gpio_id-(uint32_t)GPIOA)>>10;
	RCC_AHB1PeriphClockCmd((RCC_AHB1Periph_GPIOA<<offset), ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = pins;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = pupd; 
    // 初始化 GPIOA
    GPIO_Init(gpio_id, &GPIO_InitStructure);
}