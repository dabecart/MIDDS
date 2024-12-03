#ifndef HW_TIMERS_h
#define HW_TIMERS_h

#include <stdint.h>
#include <stdio.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"

#define CHANNEL_COUNT 4
#define COUNTS_PER_CHANNEL 32

typedef struct HWTimerChannel {
    uint64_t  triggers[COUNTS_PER_CHANNEL];
    uint8_t   triggerCount;
} HWTimerChannel;

typedef struct HWTimer {
    HWTimerChannel      ch[CHANNEL_COUNT];
    TIM_HandleTypeDef*  htim;
    uint8_t             counterID;
} HWTimer;

typedef struct HWTimers {
    HWTimer htim1;
} HWTimers;

void initHWTimers(TIM_HandleTypeDef* htim1, HWTimers* htimers);

void startHWTimers(HWTimers* htimers);

uint16_t printHWTimer(HWTimer* hwTimer, char* msg, const uint16_t maxMsgLen);

uint8_t snprintf64Hex(char* outMsg, uint8_t msgSize, uint64_t n);

// Defined in MainMCU.h
extern HWTimers hwTimers;

#endif // HW_TIMERS_h