#ifndef __VOICE_H
#define __VOICE_H

#include "stm32f10x.h"

// º¯ÊýÉùÃ÷
void Voice_Init(void);
void Voice_PlayTrack(uint16_t track_num);
void Voice_SetVolume(uint8_t vol);

#endif

