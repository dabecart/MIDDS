#include "HWTimers.h"
#include "MainMCU.h"

volatile uint64_t coarse = 0;
volatile uint32_t currentTimer = 0;
volatile HWTimerChannel* currentChannel;
volatile uint64_t currentCoarse = 0;

void initHWTimers(TIM_HandleTypeDef* htim1, HWTimers* htimers) {
    if(htimers == NULL) return;

    htimers->htim1.htim = htim1;
    htimers->htim1.counterID = 1;
}

void startHWTimers(HWTimers* htimers) {
    if(htimers == NULL) {
        return;
    }

    HAL_TIM_Base_Start_IT(htimers->htim1.htim);
    HAL_TIM_IC_Start_IT(htimers->htim1.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htimers->htim1.htim, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(htimers->htim1.htim, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(htimers->htim1.htim, TIM_CHANNEL_4);
}

uint16_t printHWTimer(HWTimer* hwTimer, char* outMsg, const uint16_t maxMsgLen) {
    uint16_t msgSize = 0;
    HWTimerChannel* hwCh = NULL;

    for(uint8_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++) {
        hwCh = hwTimer->ch + channelIndex;

        if(hwCh->triggerCount == 0) continue;

        msgSize += snprintf(outMsg + msgSize, maxMsgLen - msgSize, 
                            "C%d.%d:", hwTimer->counterID, channelIndex);

        for(uint8_t countIndex = 0; countIndex < hwCh->triggerCount; countIndex++) {
            msgSize += snprintf64Hex(
                            outMsg + msgSize, 
                            maxMsgLen - msgSize,
                            hwCh->triggers[countIndex]
                        );
        }
        msgSize += snprintf(outMsg + msgSize, maxMsgLen-msgSize, "\n");
    
        // Allow to receive interrupts again.
        hwCh->triggerCount = 0;
    }

    return msgSize;
}

// TIMER INTERRUPTS
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {
	currentCoarse = coarse;

    if(htim->Instance == hwTimers.htim1.htim->Instance) {

        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
            currentTimer = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            currentChannel = hwTimers.htim1.ch;
        }else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
            currentTimer = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
            currentChannel = hwTimers.htim1.ch+1;
        }else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
            currentTimer = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
            currentChannel = hwTimers.htim1.ch+2;
        }else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
            currentTimer = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
            currentChannel = hwTimers.htim1.ch+3;
        }else{
            return;
        }

        if(currentChannel->triggerCount >= COUNTS_PER_CHANNEL) {
            return;
        }

        currentChannel->triggers[currentChannel->triggerCount] = currentCoarse + currentTimer;
        currentChannel->triggerCount++;
    }

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        coarse += 0x10000ULL;
    }
}

uint8_t snprintf64Hex(char* outMsg, uint8_t msgSize, uint64_t n) {
    static char temp[16];
    uint8_t index = 0;
    while(n != 0) {
        temp[index] = n % 16;
        if(temp[index] <= 9) temp[index] += '0';
        else temp[index] += 'A' - 10;
        index++;
        n /= 16;
    }

    if(index > msgSize) {
        return 0;
    }

    outMsg[0] = 'x';
    for(uint8_t i = 1; i <= index; i++) {
        outMsg[i] = temp[index-i]; 
    }

    return index;
}