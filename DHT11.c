#include "DHT11.h"
#include "Delay.h"

// 引脚定义 (接在了 PB12)
#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_Pin_12

// 配置引脚为推挽输出
static void DHT11_Mode_Out(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

// 配置引脚为上拉输入
static void DHT11_Mode_In(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

// 主机复位 DHT11
static void DHT11_Reset(void) 
{
    DHT11_Mode_Out();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_ms(20); // 主机拉低至少 18ms
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30); // 主机拉高 20~40us 等待 DHT11 响应
}

// 等待 DHT11 回应
static uint8_t DHT11_Check(void) 
{
    uint8_t retry = 0;
    DHT11_Mode_In();
    
    // 等待拉低
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1 && retry < 100) {
        retry++; 
        Delay_us(1);
    }
    if(retry >= 100) return 1; 
    else retry = 0;
    
    // 等待拉高
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0 && retry < 100) {
        retry++; 
        Delay_us(1);
    }
    if(retry >= 100) return 1;
    
    return 0;
}

// 读取一个位 (Bit)
static uint8_t DHT11_Read_Bit(void) 
{
    uint8_t retry = 0;
    // 等待变为低电平
    while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1 && retry < 100) { retry++; Delay_us(1); }
    retry = 0;
    // 等待变为高电平
    while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0 && retry < 100) { retry++; Delay_us(1); }
    
    Delay_us(40); // 等待 40us 后判断电平高低
    
    if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1) return 1;
    else return 0;
}

// 读取一个字节 (Byte)
static uint8_t DHT11_Read_Byte(void) 
{
    uint8_t i, dat = 0;
    for (i = 0; i < 8; i++) 
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

// 对外接口：初始化 DHT11
uint8_t DHT11_Init(void) 
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    DHT11_Reset();
    return DHT11_Check();
}

// 对外接口：读取一次温湿度数据
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi) 
{
    uint8_t buf[5];
    uint8_t i;
    DHT11_Reset();
    if(DHT11_Check() == 0) 
    {
        for(i = 0; i < 5; i++) {
            buf[i] = DHT11_Read_Byte();
        }
        // 校验和验证数据是否正确
        if((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4]) 
        {
            *humi = buf[0];
            *temp = buf[2];
            return 0; // 读取成功
        }
    }
    return 1; // 读取失败
}

