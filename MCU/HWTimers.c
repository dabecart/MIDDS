#include "HWTimers.h"
#include "MainMCU.h"

volatile uint64_t coarse = 0;

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
inline void saveTimestamp(TIM_HandleTypeDef* htim, HWTimerChannel* channel, uint32_t channelID) {
    if(channel->triggerCount >= COUNTS_PER_CHANNEL) {
        return;
    }

    uint64_t capturedVal = HAL_TIM_ReadCapturedValue(htim, channelID);
    if((htim->Instance->SR & (TIM_FLAG_UPDATE)) == (TIM_FLAG_UPDATE) &&
       (htim->Instance->DIER & (TIM_IT_UPDATE)) == (TIM_IT_UPDATE)) {
        // If a timer reset has happened, the upper value may belong to the new coarse value which
        // will may be updated after this function returns. If the captured value is low, that means
        // that this capture belongs to the new coarse.
        if(capturedVal < __HAL_TIM_GET_COUNTER(htim)) {
            capturedVal += 0x10000ULL;
        }
    }

    channel->triggers[channel->triggerCount] = coarse + capturedVal;
    channel->triggerCount++;
}

void captureInputISR(TIM_HandleTypeDef* htim) {
    HWTimer* selectedTimer = NULL;
    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        selectedTimer = &hwTimers.htim1;
    }
    if(selectedTimer == NULL) return;

    uint32_t itEnabled = htim->Instance->DIER;
    uint32_t itFlags   = htim->Instance->SR;
    if((itFlags & (TIM_FLAG_CC1)) == (TIM_FLAG_CC1) && 
      ((itEnabled & (TIM_IT_CC1)) == (TIM_IT_CC1))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC1);
        if ((htim->Instance->CCMR1 & TIM_CCMR1_CC1S) != 0x00U){
            saveTimestamp(htim, selectedTimer->ch, TIM_CHANNEL_1);
        }
    }

    itFlags   = htim->Instance->SR;
    if((itFlags & (TIM_FLAG_CC2)) == (TIM_FLAG_CC2) && 
      ((itEnabled & (TIM_IT_CC2)) == (TIM_IT_CC2))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC2);
        if ((htim->Instance->CCMR1 & TIM_CCMR1_CC2S) != 0x00U){
            saveTimestamp(htim, selectedTimer->ch + 1, TIM_CHANNEL_2);
        }
    }

    itFlags   = htim->Instance->SR;
    if((itFlags & (TIM_FLAG_CC3)) == (TIM_FLAG_CC3) && 
      ((itEnabled & (TIM_IT_CC3)) == (TIM_IT_CC3))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC3);
        if ((htim->Instance->CCMR2 & TIM_CCMR2_CC3S) != 0x00U){
            saveTimestamp(htim, selectedTimer->ch + 2, TIM_CHANNEL_3);
        }
    }

    itFlags   = htim->Instance->SR;
    if((itFlags & (TIM_FLAG_CC4)) == (TIM_FLAG_CC4) && 
      ((itEnabled & (TIM_IT_CC4)) == (TIM_IT_CC4))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC4);
        if ((htim->Instance->CCMR2 & TIM_CCMR2_CC4S) != 0x00U){
            saveTimestamp(htim, selectedTimer->ch + 3, TIM_CHANNEL_4);
        }
    }

    itFlags   = htim->Instance->SR;
    if((itFlags & (TIM_FLAG_UPDATE)) == (TIM_FLAG_UPDATE) &&
       (itEnabled & (TIM_IT_UPDATE)) == (TIM_IT_UPDATE)) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
        coarse += 0x10000ULL;
    }
}

void restartTimerISR(TIM_HandleTypeDef* htim) {
    uint32_t itflag = htim->Instance->SR;
    if((itflag & (TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC3 | TIM_FLAG_CC4)) != 0) {
        // If any of the capture input flags are on, then the coarse will be incremented at the end 
        // of captureInputISR().
        return;
    }

    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        if((htim->Instance->SR & (TIM_FLAG_UPDATE)) == (TIM_FLAG_UPDATE) &&
           (htim->Instance->DIER & (TIM_IT_UPDATE)) == (TIM_IT_UPDATE)) {
            __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
            coarse += 0x10000ULL;
        }
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

    return index + 1;
}