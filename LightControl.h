#ifndef __LIGHT_CONTROL_H
#define __LIGHT_CONTROL_H

#include "stm32f10x.h"

// 硬件初始化：配置 PA0(TIM2_CH1) 和 PA1(TIM2_CH2)
void Light_Init(void);

// 设置目标亮度 (0-100)
void Light_White_Set(uint8_t brightness);
void Light_Yellow_Set(uint8_t brightness);

// 平滑调光更新函数，需放入主循环中高频执行
void Light_SmoothUpdate(void);

#endif

