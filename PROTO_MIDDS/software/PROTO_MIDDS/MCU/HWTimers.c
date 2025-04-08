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
#include "ChannelController.h"

volatile uint64_t coarse = 0;
volatile uint64_t newCoarse = 0;
// Time when the sync was measured by the TIMx.
volatile uint64_t lastSyncMeasured = 0;
// Time when the sync was supposed to be measured by an ideal TIMx.
volatile uint64_t lastSyncIdeal = 0;
volatile uint8_t  currentSyncState = 0xFF;   // 0 = Low, 1 = High, 0xFF = Unknown/Not being used.
volatile uint8_t  syncPulseCount = 0;

volatile uint64_t newSyncTime = -1;

void initHWTimers(HWTimers* htimers, TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, 
                  TIM_HandleTypeDef* htim3, TIM_HandleTypeDef* htim4)
{
    if(htimers == NULL) return;

    htimers->htim1 = htim1;
    htimers->htim2 = htim2;
    htimers->htim3 = htim3;
    htimers->htim4 = htim4;

    // The master time in PROTO MIDDS is the TIM1. The clock of this timer is the same being used 
    // in all other timers.
    htimers->htimMaster = htim1;

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // TIMx initialization
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    HWTimerChannel* currentChannel = htimers->channels;
    uint16_t channelNumber = 0;
    // Ch 00: TIM3_2
    initHWTimer_(currentChannel++, htim3, TIM_CHANNEL_2, CH00_GPIO_Port,  CH00_Pin, channelNumber++, 0);
    // Ch 01: TIM3_1
    initHWTimer_(currentChannel++, htim3, TIM_CHANNEL_1, CH01_GPIO_Port,  CH01_Pin, channelNumber++, 0);
    // Ch 02: TIM3_3
    initHWTimer_(currentChannel++, htim3, TIM_CHANNEL_3, CH02_GPIO_Port,  CH02_Pin, channelNumber++, 0);
    // Ch 03: TIM3_4
    initHWTimer_(currentChannel++, htim3, TIM_CHANNEL_4, CH03_GPIO_Port,  CH03_Pin, channelNumber++, 0);
    // Ch 04: TIM1_3
    initHWTimer_(currentChannel++, htim1, TIM_CHANNEL_3, CH04_GPIO_Port,  CH04_Pin, channelNumber++, 0);
    // Ch 05: TIM1_2
    initHWTimer_(currentChannel++, htim1, TIM_CHANNEL_2, CH05_GPIO_Port,  CH05_Pin, channelNumber++, 0);
    // Ch 06: TIM1_1
    initHWTimer_(currentChannel++, htim1, TIM_CHANNEL_1, CH06_GPIO_Port,  CH06_Pin, channelNumber++, 0);
    // Ch 07: TIM4_4
    initHWTimer_(currentChannel++, htim4, TIM_CHANNEL_4, CH07_GPIO_Port,  CH07_Pin, channelNumber++, 0);
    // Ch 08: TIM4_1
    initHWTimer_(currentChannel++, htim4, TIM_CHANNEL_1, CH08_GPIO_Port,  CH08_Pin, channelNumber++, 0);
    // Ch 09: TIM4_2
    initHWTimer_(currentChannel++, htim4, TIM_CHANNEL_2, CH09_GPIO_Port,  CH09_Pin, channelNumber++, 0);
    // Ch 10: TIM2_4
    initHWTimer_(currentChannel++, htim2, TIM_CHANNEL_4, CH10_GPIO_Port,  CH10_Pin, channelNumber++, 0);
    // Ch 11: TIM2_3
    initHWTimer_(currentChannel++, htim2, TIM_CHANNEL_3, CH11_GPIO_Port,  CH11_Pin, channelNumber++, 0);
    // Ch 12: TIM2_1
    initHWTimer_(currentChannel++, htim2, TIM_CHANNEL_2, CH12_GPIO_Port,  CH12_Pin, channelNumber++, 0);
    // Ch 13: TIM2_1
    initHWTimer_(currentChannel++, htim2, TIM_CHANNEL_1, CH13_GPIO_Port,  CH13_Pin, channelNumber++, 0);
}

void initHWTimer_(HWTimerChannel* timCh, TIM_HandleTypeDef* htim, uint32_t timChannel,
                 GPIO_TypeDef* gpioPort, uint32_t gpioPin, uint16_t channelNumber, uint8_t isSync) {
    init_cb64(&timCh->data, CIRCULAR_BUFFER_64_MAX_SIZE);
    
    timCh->htim = htim;
    timCh->timChannel = timChannel;
    switch (timCh->timChannel) {
        case TIM_CHANNEL_1: timCh->channelMask = TIM_FLAG_CC1; break;
        case TIM_CHANNEL_2: timCh->channelMask = TIM_FLAG_CC2; break;
        case TIM_CHANNEL_3: timCh->channelMask = TIM_FLAG_CC3; break;
        case TIM_CHANNEL_4: timCh->channelMask = TIM_FLAG_CC4; break;
        default:    break;
    }

    timCh->gpioPort = gpioPort;
    timCh->gpioPin = gpioPin;
    
    timCh->isSYNC = isSync;
    timCh->channelNumber = channelNumber;
    timCh->lastPrintTick = 0;

    timCh->lastFrequency = -1.0;
    timCh->lastDutyCycle = -1.0;
    timCh->lastFrequencyCalculationTick = 0;
}

void setSyncParameters(HWTimers* htimers, float frequency, float dutyCycle, 
                       uint32_t syncChNumber, uint64_t requestSyncTime) {
    htimers->frequencySYNC = frequency;
    htimers->dutyCycleSYNC = dutyCycle;

    // Convert Duty Cycle from percentage to [0, 1].
    dutyCycle /= 100.0;

    htimers->idealPeriodHighSYNC = MCU_FREQUENCY*dutyCycle/hwTimers.frequencySYNC;
    htimers->idealPeriodLowSYNC  = MCU_FREQUENCY*(1.0 - dutyCycle)/hwTimers.frequencySYNC;

    // Set all channels as not SYNCs.
    for(int i = 0; i < HW_TIMER_CHANNEL_COUNT; i++) {
        htimers->channels[i].isSYNC = 0;
    }

    if(syncChNumber == -1UL) {
        // Directly set the MIDDS time. By subtracting the current time, we make it so that when the
        // current TIMx value is added to the coarse, "now" would be "newSyncTime".
        coarse = requestSyncTime - __HAL_TIM_GET_COUNTER(htimers->htimMaster);
        newCoarse = coarse + 0x10000ULL;
    }else {
        Channel* syncCh = getChannelFromNumber(syncChNumber);
        if((syncCh == NULL) || (syncCh->type != CHANNEL_TIMER)) return;
        syncCh->data.timer.timerHandler->isSYNC = 1;
        newSyncTime = requestSyncTime;
    }

}

void startHWTimers(HWTimers* htimers) {
    if(htimers == NULL) return;

    // Start base timers.
    HAL_TIM_Base_Start_IT(hwTimers.htim1);
    // Enable only the Update ISR of the master timer, in the prototype version, that's TIM1.
    HAL_TIM_Base_Start_IT(hwTimers.htim2);
    __HAL_TIM_DISABLE_IT(hwTimers.htim2, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(hwTimers.htim3);
    __HAL_TIM_DISABLE_IT(hwTimers.htim3, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(hwTimers.htim4);
    __HAL_TIM_DISABLE_IT(hwTimers.htim4, TIM_IT_UPDATE);

    // First, start all timers and disable their interrupts.
    for(uint16_t i = 0; i < HW_TIMER_CHANNEL_COUNT; i++) {
        HAL_TIM_IC_Start_IT(hwTimers.channels[i].htim, hwTimers.channels[i].timChannel);
        setHWTimerEnabled(hwTimers.channels + i, 0);
    }
}

void clearHWTimer(HWTimerChannel* hwTimer) {
    empty_cb64(&hwTimer->data);
}

uint8_t readyToPrintHWTimer(HWTimerChannel* hwTimer) {
    return (hwTimer->data.len > 0) && (
                ((HAL_GetTick() - hwTimer->lastPrintTick) >= MCU_CHANNEL_PRINT_INTERVAL) ||
                (hwTimer->data.len >= CIRCULAR_BUFFER_64_MAX_SIZE/2)
            );
}

void setHWTimerEnabled(HWTimerChannel* hwTimer, uint8_t enabled){
    if(enabled) {
        __HAL_TIM_ENABLE_IT(hwTimer->htim, hwTimer->channelMask);
    }else {
        __HAL_TIM_DISABLE_IT(hwTimer->htim, hwTimer->channelMask);
    }
}

void getChannelFrequencyAndDutyCycle(HWTimerChannel* hwTimer, 
                                     double* frequency, double* dutyCycle) {
    
    if(hwTimer->data.len < HW_TIMER_MIN_SAMPLES_NEEDED) {
        if((HAL_GetTick() - hwTimer->lastFrequencyCalculationTick) > 
            HW_TIMER_TICKS_UNTIL_FREQ_RECALCULATE) {
            hwTimer->lastFrequency = -1.0;
            hwTimer->lastDutyCycle = -1.0;
        }

        *frequency = hwTimer->lastFrequency;
        *dutyCycle = hwTimer->lastDutyCycle;
        return;
    }

    // This function can only be called on "Input" mode channels.
    double periodSum = 0, risedTimeSum = 0;
    uint64_t previousRisingTime = 0;
    uint32_t cycleCount = 0;
    uint8_t firstRising = 1;

    hwTimer->data.locked = 1;
    uint64_t timestamp;
    while(hwTimer->data.len > 0) {
        if(!pop_cb64(&hwTimer->data, &timestamp)) break;

        uint64_t time = timestamp >> 1;
        uint32_t isRising = timestamp & 0x01ULL;

        if(!isRising) {
            // Continue until a rising edge is found.
            if(firstRising) continue;

            // Break the loop if the last edge is a falling edge. The calculations must end on a 
            // rising edge.
            if(hwTimer->data.len == 0) break;
        }

        if(isRising) {
            if(!firstRising) {
                periodSum += time - previousRisingTime;
                cycleCount++;
            }
            previousRisingTime = time;
            firstRising = 0;
        }else {
            risedTimeSum += time - previousRisingTime;
        }
    }

    hwTimer->data.locked = 0;

    if(cycleCount > 0) {
        *frequency = ((double) MCU_FREQUENCY) * cycleCount / periodSum;
        *dutyCycle = risedTimeSum * 100.0 / periodSum;
    }else {
        *frequency = -1.0;
        *dutyCycle = -1.0;
    }

    hwTimer->lastFrequency = *frequency;
    hwTimer->lastDutyCycle = *dutyCycle;
    hwTimer->lastFrequencyCalculationTick = HAL_GetTick();
}

uint64_t getMIDDSTime(HWTimers* htimers) {
    return coarse + __HAL_TIM_GET_COUNTER(htimers->htimMaster);
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// TIMER ISR FUNCTIONS
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

inline void saveTimestamp_(HWTimerChannel* channel, uint8_t addCoarseIncrement) {
    if(((channel->htim->Instance->SR & channel->channelMask) == 0) || 
       ((channel->htim->Instance->DIER & channel->channelMask) == 0))
    {
        // If there's no capture input for this channel or is it not enabled, skip it.
        return;
    }

    __HAL_TIM_CLEAR_FLAG(channel->htim, channel->channelMask);

    uint64_t capturedVal = HAL_TIM_ReadCapturedValue(channel->htim, channel->timChannel);
    // If during this function a restart event has ocurred (addCoarseIncrement = 1), then the 
    // captured value may belong to the new coarse which hasn't been updated still. If the captured
    // value is smaller than the current timer, then the captured value belongs to the new coarse
    // counter value (which still hasn't been updated).
    if(addCoarseIncrement && (capturedVal < __HAL_TIM_GET_COUNTER(channel->htim))){
        capturedVal += newCoarse;
    }else {
        capturedVal += coarse;
    }

    uint64_t currentGPIOValue = (channel->gpioPort->IDR & channel->gpioPin) != 0;
    if(channel->isSYNC) {
        if(currentGPIOValue) {
            // If the current GPIO level is HIGH, that means that the LOW period has just occurred.
            hwTimers.measuredPeriodLowSYNC = capturedVal - lastSyncMeasured;

            if(newSyncTime != -1) {
                // The computer has requested to set the MIDDS time.
                
                // By subtracting the current time, we make it so that when the current TIMx value
                // is added to the coarse, "now" would be "newSyncTime".
                coarse = newSyncTime - __HAL_TIM_GET_COUNTER(channel->htim);
                if(addCoarseIncrement) {
                    // This request is happening during a master timer update. Recalculate the new
                    // coarse.
                    newCoarse = coarse + 0x10000ULL;
                }

                lastSyncIdeal = newSyncTime;

                newSyncTime = -1;

                // Set the SYNC as not synchronized.
                currentSyncState = 0xFF;
                syncPulseCount = -1;
            }else {
                lastSyncIdeal += hwTimers.idealPeriodLowSYNC;
            }
        }else {
            // If the current GPIO level is LOW, that means that the HIGH period has just occurred.
            hwTimers.measuredPeriodHighSYNC = capturedVal - lastSyncMeasured;
            lastSyncIdeal += hwTimers.idealPeriodHighSYNC;
        }
        lastSyncMeasured = capturedVal;

        if(syncPulseCount >= HW_TIMER_GOOD_SYNCS_UNTIL_SYNCHRONIZED){
            currentSyncState = currentGPIOValue;
        }else{
            syncPulseCount++;
        }
    }
    
    // Apply SYNC corrections only if SYNC is ready.
    if(currentSyncState != 0xFF){
        // SYNC corrections are an interpolation between the previous HIGH or LOW measured period
        // and the ideal periods that would give an ideal clock.
        // captureMeasured - lastSyncMeasured   measuredSyncPeriod
        // ---------------------------------- = ------------------ --> Calculate captureIdeal.
        //    captureIdeal - lastSyncIdeal        realSyncPeriod
        if(capturedVal >= lastSyncMeasured) {
            // Enters here if this capture happened AFTER the current SYNC pulse.
            if(currentSyncState == 0) {
                capturedVal = lastSyncIdeal + 
                              hwTimers.idealPeriodLowSYNC*(capturedVal - lastSyncMeasured)/
                              hwTimers.measuredPeriodLowSYNC;
            }else {
                capturedVal = lastSyncIdeal + 
                              hwTimers.idealPeriodHighSYNC*(capturedVal - lastSyncMeasured)/
                              hwTimers.measuredPeriodHighSYNC;
            }
        }else {
            // Enters here if this capture happened BEFORE the current SYNC pulse. Same formulas but
            // with a little of unsigned math magic.
            if(currentSyncState == 0) {
                capturedVal = lastSyncIdeal - 
                              hwTimers.idealPeriodLowSYNC*(lastSyncMeasured - capturedVal)/
                              hwTimers.measuredPeriodLowSYNC;
            }else {
                capturedVal = lastSyncIdeal -
                              hwTimers.idealPeriodHighSYNC*(lastSyncMeasured - capturedVal)/
                              hwTimers.measuredPeriodHighSYNC;
            }
        }
    }

    // TODO: Make currentSyncState = 0xFF if enough time has passed since the last SYNC.
    
    // Move the value to the left one bit. The LSB will signal the current state of the GPIO.
    capturedVal = (capturedVal << 1) | currentGPIOValue;
    push_cb64(&channel->data, capturedVal);
}

void captureInputISR_(TIM_HandleTypeDef* htim) {
    for(uint16_t i = 0; i < HW_TIMER_CHANNEL_COUNT; i++) {
        if(htim->Instance != hwTimers.channels[i].htim->Instance) continue;

        // Don't add the coarse increment. That's only done when a timer update has occurred.
        saveTimestamp_(hwTimers.channels+i, 0);
    }
}

void restartMasterTimerISR_(TIM_HandleTypeDef* htim) {
    // This function is only called by TIM1 (the master timer).
    // Even though captureInputISR_() and this function have the same priority in NVIC, this 
    // function interrupts the previous one, I suppose it has something to do with hardware 
    // priority.
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
    for(uint16_t i = 0; i < HW_TIMER_CHANNEL_COUNT; i++) {
        saveTimestamp_(hwTimers.channels + i, 1);
    }
    
    // After all channels have been updated, modify the coarse.
    coarse = newCoarse;
    
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
}
