#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdint.h>
#include "stm32f4xx_gpio.h"

void init_output_pins(GPIO_TypeDef *gpio_id, uint16_t pins);
void init_input_pins(GPIO_TypeDef *gpio_id, uint16_t pins, GPIOPuPd_TypeDef pupd);

#endif