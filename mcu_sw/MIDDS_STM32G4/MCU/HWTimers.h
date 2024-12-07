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

#define HW_TIMER_CHANNEL_COUNT 4

// Related data and timestamps of a single Hardware Timer.
typedef struct HWTimerChannel {
    CircularBuffer64  data;
    GPIO_TypeDef* gpioPort;
    uint32_t gpioPin;
} HWTimerChannel;

// The descriptors of a Hardware Timer.
typedef struct HWTimer {
    HWTimerChannel      ch[HW_TIMER_CHANNEL_COUNT];
    TIM_HandleTypeDef*  htim;
    uint8_t             counterID;
} HWTimer;

// Collection of all Hardware Timers.
typedef struct HWTimers {
    HWTimer htim1;
    HWTimer htim2;
    HWTimer htim3;
    HWTimer htim4;
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
 * @brief Initialize all ISR related to the Hardware Timers.
 * @param htimers. Pointer to the HWTimers struct containing all data related to timers.
***************************************************************************************************/
void startHWTimers(HWTimers* htimers);

/**************************************** FUNCTION *************************************************
 * @brief Prints the content of a HWTimer to a string.
 * @param hwTimer. Pointer to the HWTimer to print.
 * @param msg. Pointer to the string to print to.
 * @param maxMsgLen. Length of the msg buffer.
 * @return uint16_t. The number of characters written to msg.
***************************************************************************************************/
uint16_t sprintfHWTimer(HWTimer* hwTimer, char* msg, const uint16_t maxMsgLen);

/**************************************** FUNCTION *************************************************
 * @brief Check if there's enough data to print a HWTimer.
 * @param hwTimer. Pointer to the HWTimer to check.
 * @return uint8_t. 0 if not ready, 1 if ready.
***************************************************************************************************/
uint8_t readyToPrintHWTimer(HWTimer* hwTimer);

/**************************************** FUNCTION *************************************************
 * @brief Converts a uint64_t number into HEX. This number gets written into a string. The written
 * number will always start with x. If the HEX number exceeds msgSize, nothing gets written.
 * @param outMsg. Buffer to write the HEX number to.
 * @param msgSize. Length of outMsg buffer.
 * @param n. The number to convert into HEX.
 * @return uint16_t. The number of bytes written to outMsg.
***************************************************************************************************/
uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n);

/**************************************** FUNCTION *************************************************
 * @brief Gets the stored value in a TIM capture input register and stores it in the related 
 * HWTimer chanel circular buffer.
 * @param htim. Timer handler that is requesting the ISR.
 * @param channel. The HWTimerChannel pointer related to the ISR.
 * @param channelID. The hardware ID of the channel (TIM_CHANNEL_x, x is 1 to 4).
 * @param addCoarseIncrement. If set, this function is being called from a reset timer event so it
 * checks if the current captured value belongs to the current coarse value or the next one.
***************************************************************************************************/
void saveTimestamp(TIM_HandleTypeDef* htim, HWTimerChannel* channel, 
                   uint32_t channelID, uint8_t addCoarseIncrement);

/**************************************** FUNCTION *************************************************
 * @brief Checks all capture input channels of a HWTimer. If any of them contains a capture event, 
 * saveTimestamp() gets called for that channel. 
 * @param timer. Timer handler that is requesting the ISR.
 * @param addCoarseIncrement. If set, this function is being called from a reset timer event so it
 * checks if the current captured value belongs to the current coarse value or the next one.
***************************************************************************************************/
void checkAllChannelsTimestamps(HWTimer* timer, uint8_t addCoarseIncrement);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Capture Input Event (when an edge has triggered a TIM and there's
 * a new timestamp to process).
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void captureInputISR(TIM_HandleTypeDef* htim);

/**************************************** FUNCTION *************************************************
 * @brief ISR function called on a Reset Event (when the TIM goes back to 0, either because and 
 * overflow or external reset).
 * @param htim. Timer handler that is requesting the ISR.
***************************************************************************************************/
void restartTimerISR(TIM_HandleTypeDef* htim);

// Defined in MainMCU.h
extern HWTimers hwTimers;

#endif // HW_TIMERS_h