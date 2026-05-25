#include "LightControl.h"

// 当前亮度和目标亮度缓存区
static uint8_t current_white = 0;
static uint8_t target_white = 0;
static uint8_t current_yellow = 0;
static uint8_t target_yellow = 0;

static uint8_t delay_cnt = 0;

/**
  * @brief  双通道 PWM 灯光初始化 (基于 TIM2)
  * PA0 -> 白光 (TIM2_CH1)
  * PA1 -> 黄光 (TIM2_CH2)
  */
void Light_Init(void)
{
    // 1. 开启时钟：TIM2挂载在 APB1，GPIOA挂载在 APB2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 2. 配置 PA0 和 PA1 为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; // 修正为你设定的 PA0 和 PA1
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 配置 TIM2 基础参数 (频率控制) 100级调光
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 100 - 1;       // 自动重装载值 ARR=100
    TIM_TimeBaseStructure.TIM_Prescaler = 720 - 1;    // 预分频器 PSC
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 4. 配置 TIM2 的 PWM 输出通道 (CH1 和 CH2)
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                // 初始占空比为0 (开机全灭)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    TIM_OC1Init(TIM2, &TIM_OCInitStructure); // 初始化通道1 -> 映射到 PA0 (白光)
    TIM_OC2Init(TIM2, &TIM_OCInitStructure); // 初始化通道2 -> 映射到 PA1 (黄光)
    
    // 5. 使能通用定时器 TIM2 
    // (注意：通用定时器不需要 TIM_CtrlPWMOutputs 这个高级定时器特有的函数)
    TIM_Cmd(TIM2, ENABLE);
}

// 设定白光目标亮度 (0-100)
void Light_White_Set(uint8_t brightness) {
    if(brightness > 100) brightness = 100;
    target_white = brightness;
}

// 设定黄光目标亮度 (0-100)
void Light_Yellow_Set(uint8_t brightness) {
    if(brightness > 100) brightness = 100;
    target_yellow = brightness;
}

/**
  * @brief  无极调光与色温平滑渐变函数 (放入 main 的 while(1) 中)
  */
void Light_SmoothUpdate(void)
{
    delay_cnt++;
    if (delay_cnt >= 2) // 改变这个数值可以调节呼吸变光的速度
    {
        delay_cnt = 0;
        
        // 白光平滑追随 (PA0)
        if (current_white < target_white) current_white++;
        else if (current_white > target_white) current_white--;
        TIM_SetCompare1(TIM2, current_white); // 修改为 TIM2_CH1
        
        // 黄光平滑追随 (PA1)
        if (current_yellow < target_yellow) current_yellow++;
        else if (current_yellow > target_yellow) current_yellow--;
        TIM_SetCompare2(TIM2, current_yellow); // 修改为 TIM2_CH2
    }
}

