#ifndef HW_TIMERS_h
#define HW_TIMERS_h

#include <stdint.h>
#include <stdio.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"
#include "CircularBuffer64.h"

#define CHANNEL_COUNT 4

typedef struct HWTimerChannel {
    // uint64_t  triggers[COUNTS_PER_CHANNEL];
    CircularBuffer64  data;
    GPIO_TypeDef* gpioPort;
    uint32_t gpioPin;
} HWTimerChannel;

typedef struct HWTimer {
    HWTimerChannel      ch[CHANNEL_COUNT];
    TIM_HandleTypeDef*  htim;
    uint8_t             counterID;
} HWTimer;

typedef struct HWTimers {
    HWTimer htim1;
    HWTimer htim2;
    HWTimer htim3;
    HWTimer htim4;
} HWTimers;

void initHWTimers(HWTimers* htimers, 
                  TIM_HandleTypeDef* htim1, 
                  TIM_HandleTypeDef* htim2, 
                  TIM_HandleTypeDef* htim3, 
                  TIM_HandleTypeDef* htim4
                 );

void startHWTimers(HWTimers* htimers);

uint16_t printHWTimer(HWTimer* hwTimer, char* msg, const uint16_t maxMsgLen);

uint8_t readyToPrintHWTimer(HWTimer* hwTimer);

uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n);

void saveTimestamp(TIM_HandleTypeDef* htim, HWTimerChannel* channel, uint32_t channelID, uint8_t addCoarseIncrement);
void checkAllChannelsTimestamps(HWTimer* timer, uint8_t addCoarseIncrement);
void captureInputISR(TIM_HandleTypeDef* htim);
void restartTimerISR(TIM_HandleTypeDef* htim);

// Defined in MainMCU.h
extern HWTimers hwTimers;

#endif // HW_TIMERS_h