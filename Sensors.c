#include "Sensors.h"

/**
  * @brief  初始化传感器相关的 GPIO 和 ADC (光敏 PA4, 红外避障 PA5)
  */
void Sensors_Init(void)
{
    // 1. 开启时钟 (ADC1, GPIOA)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // ADC 时钟 72MHz/6 = 12MHz

    GPIO_InitTypeDef GPIO_InitStructure;

    // 2. 配置 PA4 为模拟输入 (光敏电阻 LDR)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 配置 PA5 为上拉输入 (红外避障模块 DO 接这里)
    // 使用上拉输入可以防止模块没接好时引脚电平乱跳
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. ADC 基础配置
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE); // 使能 ADC1

    // 5. ADC 校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  读取光敏电阻的模拟值
  * @return 0-4095 (数值越小表示环境越亮)
  */
uint16_t LDR_GetBrightness(void)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

/**
  * @brief  读取红外避障模块状态 (原人体红外接口)
  * @return 1:有人在位, 0:无人
  */
uint8_t PIR_IsHumanPresent(void)
{
    // 注意逻辑反转：
    // 红外避障模块检测到前方有遮挡物（红外线反射回来）时，DO 会拉低（Bit_RESET）
    // 如果没有遮挡物，DO 保持高电平
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == Bit_RESET) 
    {
        return 1; // 返回 1 告诉系统“有人”
    } 
    else 
    {
        return 0; // 返回 0 告诉系统“没人”
    }
}