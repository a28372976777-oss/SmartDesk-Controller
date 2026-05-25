#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LightControl.h"
#include "Sensors.h"
#include "ultrasonic.h"
#include "DHT11.h"
#include "Voice.h"
#include "Key.h"
#include "esp8266.h"  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern char RxBuffer[];
extern uint8_t RxCounter;

#define MODE_AUTO    0
#define MODE_MANUAL  1

int main(void)
{
    // ================= 1. 初始化 =================
    OLED_Init();         
    Light_Init();        
    Sensors_Init();      
    Ultrasonic_Init();   
    DHT11_Init();        
    Voice_Init();        
    Key_Init();          
    ESP8266_UART_Init(115200); 

    OLED_ShowString(1, 1, "Smart Desk V3.0");
    Voice_PlayTrack(1);  
    Delay_ms(1500);      
    OLED_Clear();

    // 变量定义
    uint16_t ldr = 0;
    uint8_t human = 0;
    float dist = 0;
    uint8_t temp = 0, humi = 0;
    
    uint8_t sys_mode = MODE_AUTO; 
    uint8_t manual_color = 0;      // 0:白, 1:黄
    uint8_t bright_val = 30;       // 手动模式初始亮度
    uint8_t manual_step = 0;       
    
    uint8_t auto_relax = 0;        
    uint16_t study_sec = 0;
    uint16_t loop_cnt = 0;
    uint8_t warn_lock = 0; 

    while (1)
    {
        Light_SmoothUpdate(); // 必须放在最外面，保证渐变平滑
        
        loop_cnt++;
        if (loop_cnt >= 1000) loop_cnt = 0; 
       
        // --- A. 接收并解析来自 App 的  远程控制指令 ---
        if (CommandReady) 
        {
            
//            OLED_ShowString(4, 1, "                "); // 清空第4行
//            OLED_ShowString(4, 1, RxBuffer);           // 打印刚才收到的指令
            
            if (strstr(RxBuffer, "CMD:MODE")) {
                sys_mode = (sys_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
                // 切换到手动时，默认一个初始亮度
                if(sys_mode == MODE_MANUAL) { bright_val = 30; manual_color = 0; }
                Voice_PlayTrack(sys_mode ? 5 : 6); 
            }
            else if (strstr(RxBuffer, "CMD:CLR")) {
                manual_color = !manual_color;
                sys_mode = MODE_MANUAL; // 手机切颜色，自动进入手动模式
            }
            else if (strstr(RxBuffer, "CMD:BRI:")) {
                bright_val = atoi(&RxBuffer[8]); 
                if (bright_val > 99) bright_val = 99;
                sys_mode = MODE_MANUAL; // 手机拉滑块，自动进入手动模式
            }
            CommandReady = 0; 
        }

        // --- B. 物理按键处理 ---
        uint8_t key_val = Key_GetNum();
        if (key_val == 2) // PB13 切换模式
        {
            sys_mode = (sys_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
            if(sys_mode == MODE_MANUAL) { bright_val = 5; manual_color = 0; manual_step = 0; }
            Voice_PlayTrack(sys_mode ? 5 : 6); 
        }
        else if (key_val == 1 && sys_mode == MODE_MANUAL) // PB14 手动循环
        {
            manual_step++;
            if (manual_step > 6) manual_step = 0;
            switch(manual_step) {
                case 0: manual_color = 0; bright_val = 5;  break; 
                case 1: manual_color = 0; bright_val = 30; break; 
                case 2: manual_color = 0; bright_val = 99; break; 
                case 3: manual_color = 1; bright_val = 5;  break; 
                case 4: manual_color = 1; bright_val = 30; break; 
                case 5: manual_color = 1; bright_val = 99; break; 
                case 6: bright_val = 0; break;                    
            }
        }

        // --- C. 传感器感知 (每100ms更新一次) ---
        if (loop_cnt % 10 == 0) 
        {
            ldr = LDR_GetBrightness();
            human = PIR_IsHumanPresent();
            
            //  修正逻辑：只有有人的时候，才启动超声波测距
            if (human == 1) 
            {
                dist = Ultrasonic_GetDistance();
                // 坐姿报警逻辑
                if (dist > 2.0f && dist < 15.0f) {
                    if (warn_lock == 0) { Voice_PlayTrack(2); warn_lock = 1; }
                } else { 
                    warn_lock = 0; 
                }
            } 
            else 
            {
                // 没人的时候，距离强制归零，解除报警锁
                dist = 0;
                warn_lock = 0; 
            }
        }
                       
        // --- D. 核心控制逻辑：人走灯灭 + 自动/手动分流 ---
        if (human == 0) 
        {
            // 优先级最高：没人就全关，并重置计时器
            Light_White_Set(0); 
            Light_Yellow_Set(0);
            study_sec = 0; 
            auto_relax = 0;
        }
        else 
        {
            if (sys_mode == MODE_AUTO) 
            {
                // 自动模式：根据环境光计算目标亮度
                uint8_t target = (ldr > 600) ? (ldr / 40) : 0;
                if (target > 99) target = 99;

                // 根据护眼计时状态切换颜色
                if (auto_relax == 0) {
                    Light_White_Set(target); Light_Yellow_Set(0);
                } else {
                    Light_White_Set(0); Light_Yellow_Set(target);
                }
            } 
            else 
            {
                // 手动模式：执行 App 或 按键设定的值
                if (manual_color == 0) {
                    Light_White_Set(bright_val); Light_Yellow_Set(0);
                } else {
                    Light_White_Set(0); Light_Yellow_Set(bright_val);
                }
            }
        }
        
        // --- E. 计时与数据上传 (每100ms更新一次计时，每3秒传一次数据) ---
        if (loop_cnt % 100 == 0) 
        {
            // 1. 计时逻辑（保持 1 秒执行 1 次，不影响护眼计时）
            if (sys_mode == MODE_AUTO && human == 1) {
                study_sec++;
                // 缩时测试：10秒
                if (study_sec >= 10 && auto_relax == 0) { 
                    auto_relax = 1;
                    Voice_PlayTrack(4); // 语音提醒：该休息了
                }
            }
            // 2. 数据上传给 App（增加一个降频器，改成 3 秒发一次数据）
            static uint8_t upload_timer = 0;
            upload_timer++;
            
            if (upload_timer >= 3) // 满 3 秒才让 ESP8266 发送一次！极大降低云端压力
            {
                upload_timer = 0; // 重新计时
                static uint8_t step = 0;
                
                if (step == 0) 
                { 
                    DHT11_Read_Data(&temp, &humi); 
                    ESP8266_SendValue("temp", (float)temp);
                }
                else if (step == 1) ESP8266_SendValue("humi", (float)humi);
                else if (step == 2) ESP8266_SendValue("ldr",  (float)ldr);
                else if (step == 3) ESP8266_SendValue("dist", (float)dist);
                else if (step == 4) ESP8266_SendValue("mode", (float)sys_mode);
                
                if (++step >= 5) step = 0;
            }

            // OLED 显示 (保持 1 秒刷新 1 次)
            OLED_ShowString(1, 4, "Smart Desk");
            OLED_ShowString(2, 1, (sys_mode == MODE_AUTO)?"AUT":"MAN");
            OLED_ShowString(2, 5, (manual_color == 0)?"W":"Y");
            OLED_ShowNum(2, 7, bright_val, 3);
            OLED_ShowString(2, 11, "T:"); OLED_ShowNum(2, 13, temp, 2);
            OLED_ShowString(3, 1, "LDR:"); OLED_ShowNum(3, 5, ldr, 4);
            OLED_ShowString(3, 11, "H:"); OLED_ShowNum(3, 13, humi, 2);
            OLED_ShowString(4, 1, "Dist:"); OLED_ShowNum(4, 7, (uint32_t)dist, 3);
            OLED_ShowString(4, 11, "IR:"); OLED_ShowNum(4, 14, human, 1);
        }
                     
        Delay_ms(10);    
    }
}

