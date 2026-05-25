#ifndef __SENSORS_H
#define __SENSORS_H

#include "stm32f10x.h"

// 函数声明
void Sensors_Init(void);
uint16_t LDR_GetBrightness(void); // 获取环境亮度 (0-4095)
uint8_t PIR_IsHumanPresent(void); // 判断是否有人 (1:有人, 0:无人)

#endif
