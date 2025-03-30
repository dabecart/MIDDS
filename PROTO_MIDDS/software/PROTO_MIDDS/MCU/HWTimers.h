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

#define HW_TIMER_CHANNEL_COUNT                  14
#define HW_TIMER_GOOD_SYNCS_UNTIL_SYNCHRONIZED  3
#define HW_TIMER_MIN_SAMPLES_NEEDED             10
#define HW_TIMER_TICKS_UNTIL_FREQ_RECALCULATE   30000 // ticks = ms

// Related data and timestamps of a single Hardware Timer.
typedef struct HWTimerChannel {
    TIM_HandleTypeDef*  htim;
    uint32_t            timChannel;     // TIM_CHANNEL_1-4
    uint32_t            channelMask;    // TIM_FLAG_CC1-4
    GPIO_TypeDef*       gpioPort;
    uint32_t            gpioPin;
    uint8_t             isSYNC;

    uint16_t            channelNumber;
    uint32_t            lastPrintTick;

    double              lastFrequency;
    double              lastDutyCycle;
    uint32_t            lastFrequencyCalculationTick;

    CircularBuffer64    data;
} HWTimerChannel;

// Collection of all Hardware Timers.
typedef struct HWTimers {
    TIM_HandleTypeDef*  htim1;
    TIM_HandleTypeDef*  htim2;
    TIM_HandleTypeDef*  htim3;
    TIM_HandleTypeDef*  htim4;

    // Holds a pointer to the master timer.
    TIM_HandleTypeDef* htimMaster;

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

    // Collection of all HW timers.
    HWTimerChannel channels[HW_TIMER_CHANNEL_COUNT];
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

/**************************************** FUNCTION *************************************************
 * @brief Auxiliary function that inits a single HWTimer channel.
 * @param timCh. Pointer to the HWTimerChannel struct containing all data related to this timer 
 * channel. 
 * @param htim. Handler of the TIMx.
 * @param htim2. Handler of the TIM2. Defined in main.c.
 * @param htim3. Handler of the TIM3. Defined in main.c.
 * @param htim4. Handler of the TIM4. Defined in main.c.
***************************************************************************************************/
void initHWTimer_(HWTimerChannel* timCh, TIM_HandleTypeDef* htim, uint32_t timChannel,
                  GPIO_TypeDef* gpioPort, uint32_t gpioPin, uint16_t channelNumber, uint8_t isSync);

/**************************************** FUNCTION *************************************************
 * @brief Set the SYNC signal parameters.
 * @param htimers. Pointer to the HWTimers struct containing all data related to timers.
 * @param frequency. Frequency of the SYNC signal (Hz). 
 * @param dutyCycle. Duty cycle of the SYNC signal (%).
 * @param syncChNumber. The SYNC channel number. There can only be one channel set as SYNC. If the
 * channel number is not valid, the SYNC will be set as disabled.
 * @param newSyncTime. The new sync time for the next rising pulse.
***************************************************************************************************/
void setSyncParameters(HWTimers* htimers, float frequency, float dutyCycle, 
                       uint32_t syncChNumber, uint64_t newSyncTime);

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
 * @brief Calculates the frequency and duty cycle of a hardware timer by using its timestamps. 
 * Warning: it clears the timestamps array! It should not be used on any other than input channels.
 * @param hwTimer. Pointer to the HWTimer to calculate its frequency.
 * @param frequency. Where the frequency will be stored.
***************************************************************************************************/
void getChannelFrequencyAndDutyCycle(HWTimerChannel* hwTimer, 
                                     double* frequency, double* dutyCycle);

/**************************************** FUNCTION *************************************************
 * @brief Returns the MIDDS time. This time's format is UNIX time in nanoseconds and is synchronized
 * with the SYNC input.
 * @param hwTimers. Pointer to the HWTimers.
 * @return The MIDDS time.
***************************************************************************************************/
uint64_t getMIDDSTime(HWTimers* htimers);

/**************************************** FUNCTION *************************************************
 * @brief Gets the stored value in a TIM capture input register and stores it in the related 
 * HWTimer chanel circular buffer.
 * @param htim. Timer handler that is requesting the ISR.
 * @param addCoarseIncrement. If set, this function is being called from a reset timer event so it
 * checks if the current captured value belongs to the current coarse value or the next one.
***************************************************************************************************/
void saveTimestamp_(HWTimerChannel* htim, uint8_t addCoarseIncrement);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Capture Input Event (when an edge has triggered a TIM and there's
 * a new timestamp to process).
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void captureInputISR_(TIM_HandleTypeDef* htim);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Reset Event (when the TIM goes back to 0, either because and 
 * overflow or external reset) of the master timer.
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void restartMasterTimerISR_(TIM_HandleTypeDef* htim);

#endif // HW_TIMERS_h