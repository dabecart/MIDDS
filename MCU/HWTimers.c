#include "HWTimers.h"
#include "MainMCU.h"

volatile uint64_t coarse = 0;

void initHWTimers(TIM_HandleTypeDef* htim1, HWTimers* htimers) {
    if(htimers == NULL) return;

    htimers->htim1.htim = htim1;
    htimers->htim1.counterID = 1;
    init_cb64(&htimers->htim1.ch[0].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[1].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[2].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[3].data, CIRCULAR_BUFFER_64_MAX_SIZE);
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

uint8_t readyToPrintHWTimer(HWTimer* hwTimer) {
    return (hwTimer->ch[0].data.len + hwTimer->ch[1].data.len + 
            hwTimer->ch[2].data.len + hwTimer->ch[3].data.len   ) > 15;
}

uint16_t printHWTimer(HWTimer* hwTimer, char* outMsg, const uint16_t maxMsgLen) {
    uint16_t msgSize = 0;
    HWTimerChannel* hwCh = NULL;
    uint32_t currentMessages;
#if MCU_TX_IN_ASCII
    uint64_t readVal;
#endif

    for(uint8_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++) {
        hwCh = hwTimer->ch + channelIndex;

        if(hwCh->data.len == 0) continue;

        currentMessages = hwCh->data.len;   // Make it constant at this point.
        if(currentMessages > 99) currentMessages = 99;
        msgSize += snprintf(outMsg + msgSize, maxMsgLen - msgSize, 
                            "T%d.%d.%02ld:", hwTimer->counterID, channelIndex, currentMessages);
        // This upper string must be a multiple of eight bytes long (in binary output mode).

        for(uint8_t countIndex = 0; countIndex < currentMessages; countIndex++) {
            #if MCU_TX_IN_ASCII
                pop_cb64(&hwCh->data, &readVal);
                msgSize += snprintf64Hex(
                                outMsg + msgSize, 
                                maxMsgLen - msgSize,
                                readVal
                            );
            #else
                pop_cb64(&hwCh->data, (uint64_t*) (outMsg + msgSize));
                msgSize += 8;
            #endif
        }
        #if MCU_TX_IN_ASCII
            msgSize += snprintf(outMsg + msgSize, maxMsgLen-msgSize, "\n");
        #endif
    }

    return msgSize;
}

// TIMER INTERRUPTS
inline void saveTimestamp(TIM_HandleTypeDef* htim, HWTimerChannel* channel, 
                          uint32_t channelID, uint8_t addCoarseIncrement) {
    if(channel->data.len >= channel->data.size) {
        return;
    }

    uint64_t capturedVal = HAL_TIM_ReadCapturedValue(htim, channelID);
    // If during this function a restart event has ocurred (addCoarseIncrement = 1), then the 
    // captured value may belong to the new coarse which hasn't been updated still. If the captured
    // value is smaller than the current timer, then the captured value belongs to the new coarse
    // counter value (which still hasn't been updated).
    if(addCoarseIncrement && (capturedVal < __HAL_TIM_GET_COUNTER(htim))) capturedVal += 0x10000ULL;
    push_cb64(&channel->data, capturedVal + coarse);
}

void captureInputISR(TIM_HandleTypeDef* htim) {
    HWTimer* selectedTimer = NULL;
    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        selectedTimer = &hwTimers.htim1;
    }
    if(selectedTimer == NULL) return;

    uint32_t itFlags   = htim->Instance->SR;
    uint32_t itEnabled = htim->Instance->DIER;
    if((itFlags & (TIM_FLAG_CC1)) == (TIM_FLAG_CC1) && 
      ((itEnabled & (TIM_IT_CC1)) == (TIM_IT_CC1))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC1);
        saveTimestamp(htim, selectedTimer->ch, TIM_CHANNEL_1, 0);
    }

    if((itFlags & (TIM_FLAG_CC2)) == (TIM_FLAG_CC2) && 
      ((itEnabled & (TIM_IT_CC2)) == (TIM_IT_CC2))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC2);
        saveTimestamp(htim, selectedTimer->ch + 1, TIM_CHANNEL_2, 0);
    }

    if((itFlags & (TIM_FLAG_CC3)) == (TIM_FLAG_CC3) && 
      ((itEnabled & (TIM_IT_CC3)) == (TIM_IT_CC3))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC3);
        saveTimestamp(htim, selectedTimer->ch + 2, TIM_CHANNEL_3, 0);
    }

    if((itFlags & (TIM_FLAG_CC4)) == (TIM_FLAG_CC4) && 
      ((itEnabled & (TIM_IT_CC4)) == (TIM_IT_CC4))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC4);
        saveTimestamp(htim, selectedTimer->ch + 3, TIM_CHANNEL_4, 0);
    }
}

void restartTimerISR(TIM_HandleTypeDef* htim) {
    HWTimer* selectedTimer = NULL;
    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        selectedTimer = &hwTimers.htim1;
    }
    if(selectedTimer == NULL) return;

    // Before adding the coarse, check if there are any pending Capture Inputs on the timer 
    // channels. If there are, save them but take into account that an clock reset has happened and
    // the captured value may pertain to the new coarse, which still hasn't been incremented.
    uint32_t itFlags   = htim->Instance->SR;
    uint32_t itEnabled = htim->Instance->DIER;
    if((itFlags & (TIM_FLAG_CC1)) == (TIM_FLAG_CC1) && 
       ((itEnabled & (TIM_IT_CC1)) == (TIM_IT_CC1))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC1);
        saveTimestamp(htim, selectedTimer->ch, TIM_CHANNEL_1, 1);
    }

    if((itFlags & (TIM_FLAG_CC2)) == (TIM_FLAG_CC2) && 
      ((itEnabled & (TIM_IT_CC2)) == (TIM_IT_CC2))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC2);
        saveTimestamp(htim, selectedTimer->ch + 1, TIM_CHANNEL_2, 1);
    }

    if((itFlags & (TIM_FLAG_CC3)) == (TIM_FLAG_CC3) && 
      ((itEnabled & (TIM_IT_CC3)) == (TIM_IT_CC3))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC3);
        saveTimestamp(htim, selectedTimer->ch + 2, TIM_CHANNEL_3, 1);
    }

    if((itFlags & (TIM_FLAG_CC4)) == (TIM_FLAG_CC4) && 
      ((itEnabled & (TIM_IT_CC4)) == (TIM_IT_CC4))) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC4);
        saveTimestamp(htim, selectedTimer->ch + 3, TIM_CHANNEL_4, 1);
    }

    // After all channels have been updated, increase the coarse.
    if((itFlags & (TIM_FLAG_UPDATE)) == (TIM_FLAG_UPDATE) &&
        (itEnabled & (TIM_IT_UPDATE)) == (TIM_IT_UPDATE)) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
        coarse += 0x10000ULL;
    }
}

inline uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n) {
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

    // Add 1 more for the 'x' on [0].
    return index + 1;
}
