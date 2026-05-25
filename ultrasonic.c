#include "ultrasonic.h"
#include "Delay.h"

// 假设超声波 Trig 接 PA6，Echo 接 PA7
#define ULTRASONIC_PORT GPIOA
#define TRIG_PIN GPIO_Pin_6
#define ECHO_PIN GPIO_Pin_7

/**
  * @brief  初始化超声波引脚，并配置 TIM3 作为微秒级硬件计数器
  */
void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // 1. 开启 GPIOA 和 TIM3 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 2. Trig (PA6) 推挽输出
    GPIO_InitStructure.GPIO_Pin = TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ULTRASONIC_PORT, &GPIO_InitStructure);

    // 3. Echo (PA7) 浮空输入
    GPIO_InitStructure.GPIO_Pin = ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ULTRASONIC_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(ULTRASONIC_PORT, TRIG_PIN);

    // 4. 配置 TIM3 作为秒表 (1MHz 频率，即计数 1 次就是 1 微秒)
    // 72MHz / 72 = 1MHz
    TIM_TimeBaseStructure.TIM_Period = 65535;         // 最大计数值
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;     // 预分频器 
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    TIM_Cmd(TIM3, DISABLE); // 先不启动定时器
}

/**
  * @brief  使用硬件定时器获取精准距离
  * @return 距离 (单位: cm)。如果返回 -1 或 -2 表示测距超时/错误
  */
float Ultrasonic_GetDistance(void)
{
    uint32_t time_us = 0;
    uint32_t timeout_cnt = 0;
    float distance = 0;

    // 1. 发送 15us 的高电平触发信号
    GPIO_SetBits(ULTRASONIC_PORT, TRIG_PIN);
    Delay_us(15);
    GPIO_ResetBits(ULTRASONIC_PORT, TRIG_PIN);

    // 2. 等待 Echo 变高 (加入了防死机超时机制)
    timeout_cnt = 0;
    while (GPIO_ReadInputDataBit(ULTRASONIC_PORT, ECHO_PIN) == 0)
    {
        timeout_cnt++;
        Delay_us(1);
        if (timeout_cnt > 10000) return -1.0f; // 传感器没反应
    }

    // 3. 此时 Echo 刚变高，立刻清零并启动 TIM3 秒表
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);

    // 4. 等待 Echo 变低
    timeout_cnt = 0;
    while (GPIO_ReadInputDataBit(ULTRASONIC_PORT, ECHO_PIN) == 1)
    {
        timeout_cnt++;
        Delay_us(1);
        if (timeout_cnt > 15000) // 超出测量范围 (大约10米)
        {
            TIM_Cmd(TIM3, DISABLE);
            return -2.0f; 
        }
    }

    // 5. 此时 Echo 变低，立刻停止 TIM3 并读取微秒数
    TIM_Cmd(TIM3, DISABLE);
    time_us = TIM_GetCounter(TIM3);

    // 6. 计算精准距离： 声音速度 340m/s = 0.034cm/us
    // 往返距离除以2： time_us * 0.034 / 2 = time_us * 0.017
    distance = (float)time_us * 0.017f;

    return distance;
}

