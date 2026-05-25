#ifndef __ESP8266_H
#define __ESP8266_H 

#include "stm32f10x.h"

// ================= 接收指令相关定义 =================
#define RX_BUF_SIZE 64

// 声明外部变量，方便 main.c 调用
extern char RxBuffer[RX_BUF_SIZE];
extern uint8_t CommandReady;

// ================= 函数声明 =================

/**
 * @brief  初始化 USART3 (PB10/PB11) 用于 ESP8266 通讯
 * @param  baudrate: 波特率 (建议 115200)
 */
void ESP8266_UART_Init(uint32_t baudrate); 

/**
 * @brief  通过串口发送字符串
 */
void ESP_SendString(char *str);

/**
 * @brief  向 Blinker App 发送键值对数据
 * @param  key: 组件的数据键名 (Data Key)
 * @param  value: 要显示的数值
 */
void ESP8266_SendValue(char* key, float value);

#endif

