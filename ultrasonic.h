#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f10x.h"

// 引脚定义（你可以根据需要修改）
#define ULTRASONIC_GPIO_PORT    GPIOA
#define ULTRASONIC_TRIG_PIN     GPIO_Pin_3   // Trig 触发引脚
#define ULTRASONIC_ECHO_PIN     GPIO_Pin_4   // Echo 接收引脚
#define ULTRASONIC_GPIO_CLK     RCC_APB2Periph_GPIOA

// 函数声明
void Ultrasonic_Init(void);                 // 初始化超声波模块
float Ultrasonic_GetDistance(void);         // 获取距离（单位：cm）
uint32_t Ultrasonic_GetPulseWidth(void);    // 获取回响脉冲宽度（单位：us）

#endif


