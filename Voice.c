#include "Voice.h"
#include "Delay.h"

/**
  * @brief  初始化 USART2 用于控制 JQ8900 (接 PA2)
  */
void Voice_Init(void)
{
    // 1. 开启时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 2. PA2 (TX2) 配置为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. USART2 配置 (JQ8900波特率固定9600)
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx; // 单向发指令即可
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &USART_InitStructure);
    
    // 4. 开启串口
    USART_Cmd(USART2, ENABLE);
    
    // 开机默认将音量设置为 25 级 (最大30级)
    Delay_ms(200);
    Voice_SetVolume(20);
    Delay_ms(200);
}

/**
  * @brief  内部函数：底层串口发送一个字节
  */
static void Voice_SendByte(uint8_t byte)
{
    USART_SendData(USART2, byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/**
  * @brief  发送播放指定曲目指令
  * @param  track_num: 曲目号 (1 ~ 65535)
  */
void Voice_PlayTrack(uint16_t track_num)
{
    uint8_t cmd[6];
    
    // 指令格式: AA 07 02 曲目高 曲目低 校验和 
    cmd[0] = 0xAA; 
    cmd[1] = 0x07; 
    cmd[2] = 0x02; 
    cmd[3] = (uint8_t)(track_num >> 8); 
    cmd[4] = (uint8_t)(track_num & 0xFF);  
    cmd[5] = (uint8_t)(cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4]); 
    
    for (uint8_t i = 0; i < 6; i++) {
        Voice_SendByte(cmd[i]);
    }
}

/**
  * @brief  发送音量设置指令
  * @param  vol: 音量大小 (0 ~ 30)
  */
void Voice_SetVolume(uint8_t vol)
{
    if(vol > 30) vol = 30; // 封顶保护
    
    uint8_t cmd[5];
    // 指令格式: AA 13 01 VOL 校验和 
    cmd[0] = 0xAA;
    cmd[1] = 0x13;
    cmd[2] = 0x01;
    cmd[3] = vol;
    cmd[4] = (uint8_t)(cmd[0] + cmd[1] + cmd[2] + cmd[3]);
    
    for (uint8_t i = 0; i < 5; i++) {
        Voice_SendByte(cmd[i]);
    }
}

