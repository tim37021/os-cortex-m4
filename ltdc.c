#include "ltdc.h"
#include "stm32f4xx.h"
#include "stm32f4xx_ltdc.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery.h"

void init_ltdc()
{
	LCD_Init();

	LTDC_Layer_InitTypeDef LTDC_Layer_InitStruct;
	LTDC_Layer_InitStruct.LTDC_HorizontalStart = 30;
	LTDC_Layer_InitStruct.LTDC_HorizontalStop = (LCD_PIXEL_WIDTH + 30 - 1);
	LTDC_Layer_InitStruct.LTDC_VerticalStart = 4;
	LTDC_Layer_InitStruct.LTDC_VerticalStop = (LCD_PIXEL_HEIGHT + 4 - 1);
	LTDC_Layer_InitStruct.LTDC_PixelFormat = LTDC_Pixelformat_RGB565;
	LTDC_Layer_InitStruct.LTDC_ConstantAlpha = 255;
	LTDC_Layer_InitStruct.LTDC_DefaultColorBlue = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorGreen = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorRed = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorAlpha = 0;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;
	LTDC_Layer_InitStruct.LTDC_CFBLineLength = ((LCD_PIXEL_WIDTH * 2) + 3);
	LTDC_Layer_InitStruct.LTDC_CFBPitch = (LCD_PIXEL_WIDTH * 2);
	LTDC_Layer_InitStruct.LTDC_CFBLineNumber = LCD_PIXEL_HEIGHT;
	LTDC_Layer_InitStruct.LTDC_CFBStartAdress = LCD_FRAME_BUFFER;
	LTDC_LayerInit(LTDC_Layer1, &LTDC_Layer_InitStruct);

	LTDC_LayerCmd(LTDC_Layer1, ENABLE);
	LTDC_LayerCmd(LTDC_Layer2, DISABLE);

	LTDC_ReloadConfig(LTDC_IMReload);

	LTDC_Cmd(ENABLE);
}