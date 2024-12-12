/***************************************************************************************************
 * @file HWTimers.c
 * @brief Handles the storage and interrupts of hardware timers.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "HWTimers.h"
#include "MainMCU.h"

volatile uint64_t coarse = 0;
volatile uint64_t newCoarse = 0;
volatile uint64_t lastSyncTime = 0;
volatile uint8_t currentSyncState = 0xFF;   // 0 = Low, 1 = High, 0xFF = Unknown/Not being used.
volatile uint8_t syncPulseCount = 0;

void initHWTimers(HWTimers* htimers, TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, 
                  TIM_HandleTypeDef* htim3, TIM_HandleTypeDef* htim4)
{
    if(htimers == NULL) return;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // TIM1 initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    htimers->htim1.htim = htim1;
    htimers->htim1.counterID = 1;
    init_cb64(&htimers->htim1.ch[0].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[1].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[2].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim1.ch[3].data, CIRCULAR_BUFFER_64_MAX_SIZE);

    htimers->htim1.ch[0].gpioPort   = GPIOA;
    htimers->htim1.ch[0].gpioPin    = GPIO_PIN_8;
    htimers->htim1.ch[1].gpioPort   = GPIOA;
    htimers->htim1.ch[1].gpioPin    = GPIO_PIN_9;
    htimers->htim1.ch[2].gpioPort   = GPIOA;
    htimers->htim1.ch[2].gpioPin    = GPIO_PIN_10;
    htimers->htim1.ch[3].gpioPort   = GPIOA;
    htimers->htim1.ch[3].gpioPin    = GPIO_PIN_11;

    // TODO: Temporary set A8 as the SYNC signal, but this should be modifiable.
    // Also, be sure that this timer is the first that gets checked if it triggered.
    htimers->htim1.ch[0].isSYNC = 1;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // TIM2 initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    htimers->htim2.htim = htim2;
    htimers->htim2.counterID = 2;
    init_cb64(&htimers->htim2.ch[0].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim2.ch[1].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim2.ch[2].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim2.ch[3].data, CIRCULAR_BUFFER_64_MAX_SIZE);

    htimers->htim2.ch[0].gpioPort   = GPIOA;
    htimers->htim2.ch[0].gpioPin    = GPIO_PIN_0;
    htimers->htim2.ch[1].gpioPort   = GPIOA;
    htimers->htim2.ch[1].gpioPin    = GPIO_PIN_1;
    htimers->htim2.ch[2].gpioPort   = GPIOB;
    htimers->htim2.ch[2].gpioPin    = GPIO_PIN_10;
    htimers->htim2.ch[3].gpioPort   = GPIOB;
    htimers->htim2.ch[3].gpioPin    = GPIO_PIN_11;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // TIM3 initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    htimers->htim3.htim = htim3;
    htimers->htim3.counterID = 3;
    init_cb64(&htimers->htim3.ch[0].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim3.ch[1].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim3.ch[2].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim3.ch[3].data, CIRCULAR_BUFFER_64_MAX_SIZE);

    htimers->htim3.ch[0].gpioPort   = GPIOA;
    htimers->htim3.ch[0].gpioPin    = GPIO_PIN_6;
    htimers->htim3.ch[1].gpioPort   = GPIOA;
    htimers->htim3.ch[1].gpioPin    = GPIO_PIN_4;
    htimers->htim3.ch[2].gpioPort   = GPIOB;
    htimers->htim3.ch[2].gpioPin    = GPIO_PIN_0;
    htimers->htim3.ch[3].gpioPort   = GPIOB;
    htimers->htim3.ch[3].gpioPin    = GPIO_PIN_1;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // TIM4 initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    htimers->htim4.htim = htim4;
    htimers->htim4.counterID = 4;
    init_cb64(&htimers->htim4.ch[0].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim4.ch[1].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    // Pin A13 is used for SWD (debug).
    // init_cb64(&htimers->htim4.ch[2].data, CIRCULAR_BUFFER_64_MAX_SIZE);
    init_cb64(&htimers->htim4.ch[3].data, CIRCULAR_BUFFER_64_MAX_SIZE);

    htimers->htim4.ch[0].gpioPort   = GPIOB;
    htimers->htim4.ch[0].gpioPin    = GPIO_PIN_6;
    htimers->htim4.ch[1].gpioPort   = GPIOA;
    htimers->htim4.ch[1].gpioPin    = GPIO_PIN_12;
    // htimers->htim4.ch[2].gpioPort   = GPIOA;
    // htimers->htim4.ch[2].gpioPin    = GPIO_PIN_13;
    htimers->htim4.ch[3].gpioPort   = GPIOB;
    htimers->htim4.ch[3].gpioPin    = GPIO_PIN_9;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // SYNC Timer initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // Initial SYNC signal considered to be 1PPS 50% duty cycle.
    setSyncParameters(htimers, 1.0, 0.5);
}

void setSyncParameters(HWTimers* htimers, float frequency, float dutyCycle) {
    htimers->frequencySYNC = frequency;
    htimers->dutyCycleSYNC = dutyCycle;

    htimers->idealPeriodHighSYNC = MCU_FREQUENCY*dutyCycle/hwTimers.frequencySYNC;
    htimers->idealPeriodLowSYNC  = MCU_FREQUENCY*(1.0 - dutyCycle)/hwTimers.frequencySYNC;
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

    HAL_TIM_Base_Start_IT(htimers->htim2.htim);
    // TIM1 triggers the restart of TIM2, 3 and 4. Therefore, the update can be handled by only 
    // TIM1.
    __HAL_TIM_DISABLE_IT(htimers->htim2.htim, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(htimers->htim2.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htimers->htim2.htim, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(htimers->htim2.htim, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(htimers->htim2.htim, TIM_CHANNEL_4);

    HAL_TIM_Base_Start_IT(htimers->htim3.htim);
    __HAL_TIM_DISABLE_IT(htimers->htim3.htim, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(htimers->htim3.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htimers->htim3.htim, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(htimers->htim3.htim, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(htimers->htim3.htim, TIM_CHANNEL_4);

    HAL_TIM_Base_Start_IT(htimers->htim4.htim);
    __HAL_TIM_DISABLE_IT(htimers->htim4.htim, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(htimers->htim4.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htimers->htim4.htim, TIM_CHANNEL_2);
    // This GPIO is used by SWD (debug).
    // HAL_TIM_IC_Start_IT(htimers->htim4.htim, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(htimers->htim4.htim, TIM_CHANNEL_4);
}

void clearHWTimer(HWTimer* hwTimer) {
    empty_cb64(&hwTimer->ch[0].data);
    empty_cb64(&hwTimer->ch[1].data);
    empty_cb64(&hwTimer->ch[2].data);
    empty_cb64(&hwTimer->ch[3].data);
}

uint8_t readyToPrintHWTimer(HWTimer* hwTimer) {
    return (hwTimer->ch[0].data.len + hwTimer->ch[1].data.len + 
            hwTimer->ch[2].data.len + hwTimer->ch[3].data.len   ) > 15;
}

uint16_t sprintfHWTimer(HWTimer* hwTimer, char* outMsg, const uint16_t maxMsgLen) {
    uint16_t msgSize = 0;
    HWTimerChannel* hwCh = NULL;
    uint32_t currentMessages;
#if MCU_TX_IN_ASCII
    uint64_t readVal;
#endif

    for(uint8_t channelIndex = 0; channelIndex < HW_TIMER_CHANNEL_COUNT; channelIndex++) {
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
                if((maxMsgLen - msgSize) <= 17) {
                    break;
                }
            #else
                pop_cb64(&hwCh->data, (uint64_t*) (outMsg + msgSize));
                msgSize += 8;
                if((maxMsgLen - msgSize) <= 8) {
                    break;
                }
            #endif
        }
        #if MCU_TX_IN_ASCII
            msgSize += snprintf(outMsg + msgSize, maxMsgLen-msgSize, "\n");
        #endif
    }

    return msgSize;
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


// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// TIMER ISR FUNCTIONS
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

inline void saveTimestamp(TIM_HandleTypeDef* htim, HWTimerChannel* channel, 
                          uint32_t channelID, uint8_t addCoarseIncrement) {
    if(channel->data.len >= channel->data.size || channel->gpioPort == NULL) {
        return;
    }

    uint64_t capturedVal = HAL_TIM_ReadCapturedValue(htim, channelID);
    // If during this function a restart event has ocurred (addCoarseIncrement = 1), then the 
    // captured value may belong to the new coarse which hasn't been updated still. If the captured
    // value is smaller than the current timer, then the captured value belongs to the new coarse
    // counter value (which still hasn't been updated).
    if(addCoarseIncrement && (capturedVal < __HAL_TIM_GET_COUNTER(htim))){
        capturedVal += newCoarse;
    }else {
        capturedVal += coarse;
    }

    uint8_t currentGPIOValue = (channel->gpioPort->IDR & channel->gpioPin) != 0;
    if(channel->isSYNC) {
        if(currentGPIOValue) {
            // If the current GPIO level is HIGH, that means that the LOW period has just occurred.
            hwTimers.measuredPeriodLowSYNC = capturedVal - lastSyncTime;
        }else {
            // If the current GPIO level is LOW, that means that the HIGH period has just occurred.
            hwTimers.measuredPeriodHighSYNC = capturedVal - lastSyncTime;
        }
        lastSyncTime = capturedVal;
        syncPulseCount++;
        if(syncPulseCount >= 2) currentSyncState = currentGPIOValue;
    }else if(currentSyncState != 0xFF){
        // Apply SYNC corrections to pins which aren't SYNC.
        // SYNC corrections are an interpolation between the previous HIGH or LOW measured period
        // and the ideal periods that would give an ideal clock.
        // measured     capturedVal - lastSyncTime      measuredPeriod
        // -------- = ------------------------------- = -------------- -> Solve for idealCapturedVal
        //  ideal     idealCapturedVal - lastSyncTime    idealPeriod
        if(capturedVal >= lastSyncTime) {
            // Enters here if this capture happened AFTER the current SYNC pulse.
            if(currentSyncState == 0) {
                capturedVal = lastSyncTime + 
                            hwTimers.idealPeriodLowSYNC*(capturedVal - lastSyncTime)/
                            hwTimers.measuredPeriodLowSYNC;
            }else {
                capturedVal = lastSyncTime + 
                            hwTimers.idealPeriodHighSYNC*(capturedVal - lastSyncTime)/
                            hwTimers.measuredPeriodHighSYNC;
            }
        }else {
            // Enters here if this capture happened BEFORE the current SYNC pulse.
            if(currentSyncState == 0) {
                capturedVal = lastSyncTime - 
                            hwTimers.idealPeriodLowSYNC*(lastSyncTime - capturedVal)/
                            hwTimers.measuredPeriodLowSYNC;
            }else {
                capturedVal = lastSyncTime -
                            hwTimers.idealPeriodHighSYNC*(lastSyncTime - capturedVal)/
                            hwTimers.measuredPeriodHighSYNC;
            }
        }
    }

    // Move the value to the left one bit. The LSB will signal the current state of the GPIO.
    capturedVal = (capturedVal << 1) | currentGPIOValue;
    push_cb64(&channel->data, capturedVal);
}

void checkAllChannelsTimestamps(HWTimer* timer, uint8_t addCoarseIncrement) {
    uint32_t itFlags   = timer->htim->Instance->SR;
    uint32_t itEnabled = timer->htim->Instance->DIER;
    if((itFlags & (TIM_FLAG_CC1)) == (TIM_FLAG_CC1) && 
       ((itEnabled & (TIM_IT_CC1)) == (TIM_IT_CC1))) {
        __HAL_TIM_CLEAR_FLAG(timer->htim, TIM_FLAG_CC1);
        saveTimestamp(timer->htim, timer->ch, TIM_CHANNEL_1, addCoarseIncrement);
    }

    if((itFlags & (TIM_FLAG_CC2)) == (TIM_FLAG_CC2) && 
      ((itEnabled & (TIM_IT_CC2)) == (TIM_IT_CC2))) {
        __HAL_TIM_CLEAR_FLAG(timer->htim, TIM_FLAG_CC2);
        saveTimestamp(timer->htim, timer->ch + 1, TIM_CHANNEL_2, addCoarseIncrement);
    }

    if((itFlags & (TIM_FLAG_CC3)) == (TIM_FLAG_CC3) && 
      ((itEnabled & (TIM_IT_CC3)) == (TIM_IT_CC3))) {
        __HAL_TIM_CLEAR_FLAG(timer->htim, TIM_FLAG_CC3);
        saveTimestamp(timer->htim, timer->ch + 2, TIM_CHANNEL_3, addCoarseIncrement);
    }

    if((itFlags & (TIM_FLAG_CC4)) == (TIM_FLAG_CC4) && 
      ((itEnabled & (TIM_IT_CC4)) == (TIM_IT_CC4))) {
        __HAL_TIM_CLEAR_FLAG(timer->htim, TIM_FLAG_CC4);
        saveTimestamp(timer->htim, timer->ch + 3, TIM_CHANNEL_4, addCoarseIncrement);
    }
}

void captureInputISR(TIM_HandleTypeDef* htim) {
    HWTimer* selectedTimer = NULL;
    if(htim->Instance == hwTimers.htim1.htim->Instance) {
        selectedTimer = &hwTimers.htim1;
    }else if(htim->Instance == hwTimers.htim2.htim->Instance) {
        selectedTimer = &hwTimers.htim2;
    }else if(htim->Instance == hwTimers.htim3.htim->Instance) {
        selectedTimer = &hwTimers.htim3;
    }else if(htim->Instance == hwTimers.htim4.htim->Instance) {
        selectedTimer = &hwTimers.htim4;
    }
    if(selectedTimer == NULL) return;

    checkAllChannelsTimestamps(selectedTimer, 0);
}

void restartMasterTimerISR(TIM_HandleTypeDef* htim) {
    // This function is only called by TIM1 (the master timer).
    // Even though captureInputISR() and this function have the same priority in NVIC, this function
    // interrupts the previous one, I suppose it has something to do with hardware priority.
    uint32_t itFlags   = htim->Instance->SR;
    uint32_t itEnabled = htim->Instance->DIER;

    if(((itFlags & TIM_FLAG_UPDATE) == TIM_FLAG_UPDATE) && 
       ((itEnabled & TIM_IT_UPDATE) == TIM_IT_UPDATE)) {
        // Doing a clock overflow reset.
        newCoarse = coarse + 0x10000ULL;
    }

    // Before adding the coarse, check if there are any pending Capture Inputs on the timer 
    // channels. If there are, save them but take into account that an clock reset has happened 
    // and the captured value may pertain to the new coarse, which still hasn't been incremented.
    checkAllChannelsTimestamps(&hwTimers.htim1, 1);
    checkAllChannelsTimestamps(&hwTimers.htim2, 1);
    checkAllChannelsTimestamps(&hwTimers.htim3, 1);
    checkAllChannelsTimestamps(&hwTimers.htim4, 1);
    
    // After all channels have been updated, modify the coarse.
    coarse = newCoarse;
    
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
}
