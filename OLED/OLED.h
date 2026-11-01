#ifndef __OLED_H__
#define __OLED_H__
#include "stm32f1xx_hal.h"

/**
 * @brief  0.96寸(128x64像素)OLED驱动
 * @param  x: 列地址 0~127
 * @param  y: 行地址 0~63
 * @note  使用OLED_Refresh刷新后才会显示！！！
 */
 
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowMap(uint8_t x, uint8_t y, const uint8_t *data, uint16_t width, uint16_t height);
void OLED_Refresh(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t frontsize);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr, uint8_t frontsize);
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t frontsize);
void OLED_ShowChinese(uint8_t x, uint8_t y, const void *data, uint8_t size, uint16_t index);

void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t index, const uint8_t *data);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode);
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_DrawSineWave_Point(uint8_t amp, uint8_t centerY, float phase);
#endif
