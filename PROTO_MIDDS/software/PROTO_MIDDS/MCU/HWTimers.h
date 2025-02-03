/***************************************************************************************************
 * @file HWTimers.h
 * @brief Handles the storage and interrupts of hardware timers.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef HW_TIMERS_h
#define HW_TIMERS_h

#include <stdint.h>
#include <stdio.h>

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"

#include "CircularBuffer64.h"

#define HW_TIMER_CHANNEL_COUNT 14

// Related data and timestamps of a single Hardware Timer.
typedef struct HWTimerChannel {
    CircularBuffer64    data;
    TIM_HandleTypeDef*  htim;
    uint32_t            timChannel;     // TIM_CHANNEL_1-4
    uint32_t            channelMask;    // TIM_FLAG_CC1-4
    GPIO_TypeDef*       gpioPort;
    uint32_t            gpioPin;
    uint8_t             isSYNC;

    uint16_t            channelNumber;             
    uint32_t            lastPrintTick;
} HWTimerChannel;

// Collection of all Hardware Timers.
typedef struct HWTimers {
    // Collection of all HW timers.
    HWTimerChannel channels[HW_TIMER_CHANNEL_COUNT];

    TIM_HandleTypeDef*  htim1;
    TIM_HandleTypeDef*  htim2;
    TIM_HandleTypeDef*  htim3;
    TIM_HandleTypeDef*  htim4;

    float       frequencySYNC;
    float       dutyCycleSYNC;
    
    // Number of increments of the high pulse of SYNC.
    uint64_t    measuredPeriodHighSYNC;
    // Number of increments of the low pulse of SYNC.
    uint64_t    measuredPeriodLowSYNC;
    // Number of increments that the high pulse of SYNC should take. Calculated from the frequency
    // and duty cycle of the SYNC signal.
    uint64_t    idealPeriodHighSYNC;
    // Number of increments that the high pulse of SYNC should take. Calculated from the frequency
    // and duty cycle of the SYNC signal.
    uint64_t    idealPeriodLowSYNC;
} HWTimers;

/**************************************** FUNCTION *************************************************
 * @brief Links the timer handlers and starts all timer structs inside a HWTimer.
 * @param htimers. Pointer to the HWTimers struct containing all data related to timers. 
 * @param htim1. Handler of the TIM1. Defined in main.c.
 * @param htim2. Handler of the TIM2. Defined in main.c.
 * @param htim3. Handler of the TIM3. Defined in main.c.
 * @param htim4. Handler of the TIM4. Defined in main.c.
***************************************************************************************************/
void initHWTimers(HWTimers* htimers, 
                  TIM_HandleTypeDef* htim1, 
                  TIM_HandleTypeDef* htim2, 
                  TIM_HandleTypeDef* htim3, 
                  TIM_HandleTypeDef* htim4
                 );

void initHWTimer(HWTimerChannel* timCh, TIM_HandleTypeDef* htim, uint32_t timChannel,
                 GPIO_TypeDef* gpioPort, uint32_t gpioPin, uint16_t channelNumber, uint8_t isSync);

/**************************************** FUNCTION *************************************************
 * @brief Set the SYNC signal parameters.
 * @param htimers. Pointer to the HWTimers struct containing all data related to timers.
 * @param frequency. Frequency of the SYNC signal. 
 * @param dutyCycle. Duty cycle of the SYNC signal.
***************************************************************************************************/
void setSyncParameters(HWTimers* htimers, float frequency, float dutyCycle);

/**************************************** FUNCTION *************************************************
 * @brief Initialize all ISR related to the Hardware Timers.
 * @param htimers. Pointer to the HWTimers struct containing all data related to timers.
***************************************************************************************************/
void startHWTimers(HWTimers* htimers);

/**************************************** FUNCTION *************************************************
 * @brief Clears all buffers of a HWTimer.
 * @param hwTimer. Pointer to the HWTimerChannel to clean.
***************************************************************************************************/
void clearHWTimer(HWTimerChannel* hwTimer);

/**************************************** FUNCTION *************************************************
 * @brief Check if there's enough data to print a HWTimerChannel.
 * @param hwTimer. Pointer to the HWTimer to check.
 * @return uint8_t. 0 if not ready, 1 if ready.
***************************************************************************************************/
uint8_t readyToPrintHWTimer(HWTimerChannel* hwTimer);

/**************************************** FUNCTION *************************************************
 * @brief Enable or disable a HWTimerChannel.
 * @param hwTimer. Pointer to the HWTimer to enable/disable.
 * @param enabled. 1 to enable, 0 to disable.
***************************************************************************************************/
void setHWTimerEnabled(HWTimerChannel* hwTimer, uint8_t enabled);

/**************************************** FUNCTION *************************************************
 * @brief Gets the stored value in a TIM capture input register and stores it in the related 
 * HWTimer chanel circular buffer.
 * @param htim. Timer handler that is requesting the ISR.
 * @param addCoarseIncrement. If set, this function is being called from a reset timer event so it
 * checks if the current captured value belongs to the current coarse value or the next one.
***************************************************************************************************/
void saveTimestamp(HWTimerChannel* htim, uint8_t addCoarseIncrement);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Capture Input Event (when an edge has triggered a TIM and there's
 * a new timestamp to process).
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void captureInputISR(TIM_HandleTypeDef* htim);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Reset Event (when the TIM goes back to 0, either because and 
 * overflow or external reset) of the master timer.
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void restartMasterTimerISR(TIM_HandleTypeDef* htim);

// Defined in MainMCU.h
extern HWTimers hwTimers;

#endif // HW_TIMERS_h