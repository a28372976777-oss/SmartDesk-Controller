#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

void Key_Init(void);

// 返回 1 代表 PB14 (按键1: 切模式/颜色)
// 返回 2 代表 PB13 (按键2: 调亮度)
// 返回 0 代表无按键按下
uint8_t Key_GetNum(void); 

#endif

