/***************************************************************************************************
 * @file ChannelController.c
 * @brief Controls the registers to set the configuration of each individual channel.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2025-03-22
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef CHANNEL_CONTROLLER_h
#define CHANNEL_CONTROLLER_h

#include "stm32g4xx_hal.h"
#include "HWTimers.h"

#include "CommsProtocol.h"

#define CH_CONT_TIMER_COUNT     14
#define CH_CONT_GPIO_COUNT      16
#define CH_COUNT (CH_CONT_TIMER_COUNT+CH_CONT_GPIO_COUNT)

// Channels can be related to TIMx or GPIOs.
typedef enum ChannelType {
    CHANNEL_TIMER = 1,
    CHANNEL_GPIO,
} ChannelType;

// Channel related to a TIMx.
typedef struct TimerChannel {
    HWTimerChannel* timerHandler;
    GPIOSignalType  signalType;
} TimerChannel;

// Channel related to a GPIO.
typedef struct GPIOChannel {
    GPIO_TypeDef*   gpioPort;
    uint32_t        gpioPin;
} GPIOChannel;

// Union of all possible channels.
typedef union ChannelData {
    TimerChannel    timer;
    GPIOChannel     gpio;
} ChannelData;

// General struct holding the data of a channel.
typedef struct Channel {
    ChannelData     data;
    ChannelType     type;
    ChannelMode     mode;
    // ChannelValue    lastValue;
} Channel;

// Struct holding all channels and its configuration.
typedef struct ChannelController {
    Channel channels[CH_COUNT];
    SPI_HandleTypeDef* hspi;
} ChannelController;

/**************************************** FUNCTION *************************************************
 * @brief Initializes a ChannelController.
 * @param chCtrl. Pointer to the channel to initialize.
 * @param hspi. Pointer to the SPI handler to command the shift registers.
***************************************************************************************************/
void initChannelController(ChannelController* chCtrl, SPI_HandleTypeDef* hspi);

/**************************************** FUNCTION *************************************************
 * @brief Applies the configuration of a channel when run.
 * @param ch. Pointer to the channel whose configuration is to be applied.
***************************************************************************************************/
void applyChannelConfiguration(Channel* ch);

/**************************************** FUNCTION *************************************************
 * @brief Sets the values of the shift registers based on the configuration of all channels.
 * @param ch. Pointer to the ChannelController with all configuration.
***************************************************************************************************/
void setShiftRegisterValues(ChannelController* chCtrl);

/**************************************** FUNCTION *************************************************
 * @brief Returns a pointer to the channel with a given channelNumber.
 * @param channelNumber. The channel number to search for.abort
 * @return The Channel if found, or NULL if not.
***************************************************************************************************/
Channel* getChannelFromNumber(uint32_t channelNumber);

/**************************************** FUNCTION *************************************************
 * @brief Initializes Channel from a TimerChannel data.
 * @param ch. Pointer to the channel to initialize.
 * @param hwTimer. Pointer to the Timer Channel.
***************************************************************************************************/
void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer);

/**************************************** FUNCTION *************************************************
 * @brief Applies the configuration of a Timer channel.
 * @param ch. Pointer to the channel whose configuration is to be applied.
***************************************************************************************************/
void applyTimerChannelConfig_(Channel* ch);

/**************************************** FUNCTION *************************************************
 * @brief Initializes Channel from GPIO data.
 * @param ch. Pointer to the channel to initialize.
 * @param gpioPort. Pointer to the GPIO port.
 * @param gpioPin. GPIO pin mask value.
***************************************************************************************************/
void initChannelFromGPIO_(Channel* ch, GPIO_TypeDef* gpioPort, uint32_t gpioPin);

/**************************************** FUNCTION *************************************************
 * @brief Applies the configuration of a GPIO channel.
 * @param ch. Pointer to the channel whose configuration is to be applied.
***************************************************************************************************/
void applyGPIOChannelConfig_(Channel* ch);

#endif // CHANNEL_CONTROLLER_h