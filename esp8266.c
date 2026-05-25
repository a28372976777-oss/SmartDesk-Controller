  #include "esp8266.h"
#include <stdio.h>
#include <string.h>

// 定义全局变量
char RxBuffer[RX_BUF_SIZE];
uint8_t RxCounter = 0;
uint8_t CommandReady = 0;

/**
 * @brief  底层串口字节发送
 */
void ESP_SendString(char *str) {
    while(*str != '\0') {
        // 等待发送寄存器为空
        while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
        USART_SendData(USART3, *str++);
    }
}

/**
 * @brief  串口 3 初始化 (PB10:TX, PB11:RX) 并开启接收中断
 */
void ESP8266_UART_Init(uint32_t baudrate) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开启 GPIOB 和 USART3 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    
    // 2. 配置 PB10 (USART3_TX) 为推挽复用输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 配置 PB11 (USART3_RX) 为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 4. USART3 基本参数配置
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);
    
    // 5. 开启 USART3 接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    
    // 6. 配置中断优先级 (NVIC)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 7. 使能串口
    USART_Cmd(USART3, ENABLE);                    
}

/**
 * @brief  格式化并发送键值对数据
 * @example ESP8266_SendValue("temp", 25.5); -> 串口输出 "temp:25.5\n"
 */
void ESP8266_SendValue(char* key, float value) {
    char buf[64];
    sprintf(buf, "%s:%.1f\n", key, value);
    ESP_SendString(buf);
}

/**
 * @brief  USART3 中断服务函数
 * 逻辑：接收字符并存入缓冲区，直到收到换行符 \n 认为指令结束
 */

void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        char res = USART_ReceiveData(USART3);
        
        if (CommandReady == 1) {
            return; 
        }

        // 如果收到换行符，代表一条控制指令接收完成
        if (res == '\n' || res == '\r') {
            if (RxCounter > 0) {
                RxBuffer[RxCounter] = '\0';
                CommandReady = 1; // 置标志位，通知 main 处理
                RxCounter = 0;
            }
        } 
        else {
            if (RxCounter < (RX_BUF_SIZE - 1)) {
                RxBuffer[RxCounter++] = res;
            }
        }
    }
}

//void USART3_IRQHandler(void) {
//    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
//        char res = USART_ReceiveData(USART3);
//        
//        // 如果收到换行符，代表一条控制指令接收完成
//        if (res == '\n' || res == '\r') {
//            if (RxCounter > 0) {
//                RxBuffer[RxCounter] = '\0';
//                CommandReady = 1; // 置标志位，通知 main 处理
//                RxCounter = 0;
//            }
//        } 
//        else {
//            if (RxCounter < (RX_BUF_SIZE - 1)) {
//                RxBuffer[RxCounter++] = res;
//            }
//        }
//        // 清除中断标志由硬件或接收操作自动完成
//    }
//}


