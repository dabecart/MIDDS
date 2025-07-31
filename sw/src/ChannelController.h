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
#include "TCA6416.h"
#include "CommsProtocol.h"

#define CH_CONT_TIMER_COUNT     HW_TIMER_CHANNEL_COUNT
#define CH_CONT_GPIO_COUNT      16
#define CH_COUNT (CH_CONT_TIMER_COUNT+CH_CONT_GPIO_COUNT)

#define CH_GPIO_EXP_5V_ADDRS  0b0100000
#define CH_GPIO_EXP_3V3_ADDRS 0b0100001
#define CH_GPIO_EXP_1V8_ADDRS 0b0100000

// Channels can be related to TIMx or GPIOs.
typedef enum ChannelType {
    CHANNEL_TIMER = 1,
    CHANNEL_GPIO,
} ChannelType;

typedef struct TimerChannel_ShiftReg {
    uint8_t rsv;
    uint8_t v1;
    uint8_t v2;
    uint8_t statusRed;      // Red LED from the STATUS.
    uint8_t statusGreen;    // Green LED from the STATUS.
    uint8_t de;
    uint8_t re;
    uint8_t isOut;          // 1: output, 0: input
} __attribute__((__packed__)) TimerChannel_ShiftReg;

// Channel related to a TIMx.
typedef struct TimerChannel {
    HWTimerChannel* timerHandler;
} TimerChannel;

// Channel related to a GPIO (controlled by a TCA6416 GPIO expander).
typedef struct GPIOChannel {
    uint16_t pinNumber; // 0 to 15
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
    GPIOProtocol    protocol;
    // ChannelValue    lastValue;
} Channel;

// Struct holding all channels and its configuration.
typedef struct ChannelController {
    Channel channels[CH_COUNT];
    SPI_HandleTypeDef* hspi;

    GPIOExpander gExp5V;
    GPIOExpander gExp3V3;
    GPIOExpander gExp1V8;
} ChannelController;

/**************************************** FUNCTION *************************************************
 * @brief Initializes a ChannelController.
 * @param chCtrl. Pointer to the channel to initialize.
 * @param hspi. Pointer to the SPI handler to command the shift registers.
 * @param hi2c1. Pointer to the I2C handler in charge of controlling the GPIO Registers.
 * @param hi2c2. Pointer to the I2C handler in charge of controlling the GPIO Registers.
 ***************************************************************************************************/
void initChannelController(ChannelController* chCtrl, SPI_HandleTypeDef* hspi, 
    I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c2);

/**************************************** FUNCTION *************************************************
 * @brief Applies the configuration of a channel when run. Remember to call to 
 * setShiftRegisterValues(&chCtrl) to apply the configuration with the shift registers.
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
 * @param channelNumber. The channel number to search for.
 * @return The Channel if found, or NULL if not.
***************************************************************************************************/
Channel* getChannelFromNumber(uint32_t channelNumber);

/**************************************** FUNCTION *************************************************
 * @brief Sets the state 0/1 of a channel.
 * @param ch. The channel to set its state.
 * @param newState. New state of the channel.
 * @return 1 if the state was set successfully.
***************************************************************************************************/
uint8_t setChannelState(Channel* ch, uint8_t newState);

/**************************************** FUNCTION *************************************************
 * @brief Gets the state 0/1 of a channel.
 * @param ch. The channel to get its state.
 * @param currentState. The current state of the channel.
 * @return 1 if the state was got successfully.
***************************************************************************************************/
uint8_t getChannelState(Channel* ch, uint8_t* currentState);

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
 * @param gpioExpPin. Pin number for the GPIO Expander.
***************************************************************************************************/
void initChannelFromGPIO_(Channel* ch, uint16_t gpioExpPin);

/**************************************** FUNCTION *************************************************
 * @brief Applies the configuration of a GPIO channel.
 * @param ch. Pointer to the channel whose configuration is to be applied.
***************************************************************************************************/
void applyGPIOChannelConfig_(Channel* ch);

GPIOExpander* getGPIOExpanderFromGPIOChannel_(Channel* ch);

#endif // CHANNEL_CONTROLLER_h