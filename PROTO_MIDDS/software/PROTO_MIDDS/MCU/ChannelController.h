#ifndef CHANNEL_CONTROLLER_h
#define CHANNEL_CONTROLLER_h

#include "stm32g4xx_hal.h"
#include "HWTimers.h"

#include "CommsProtocol.h"

#define CH_CONT_TIMER_COUNT     14
#define CH_CONT_GPIO_COUNT      16
#define CH_COUNT (CH_CONT_TIMER_COUNT+CH_CONT_GPIO_COUNT)

typedef enum ChannelType {
    CHANNEL_TIMER = 1,
    CHANNEL_GPIO,
} ChannelType;

typedef struct TimerChannel {
    HWTimerChannel* timerHandler;
    GPIOSignalType  signalType;
} TimerChannel;

typedef struct GPIOChannel {
    GPIO_TypeDef*   gpioPort;
    uint32_t        gpioPin;
} GPIOChannel;

typedef union ChannelData {
    TimerChannel    timer;
    GPIOChannel     gpio;
} ChannelData;

typedef struct Channel {
    ChannelData     data;
    ChannelType     type;
    ChannelMode     mode;
    ChannelValue    lastValue;
} Channel;

typedef struct ChannelController {
    Channel channels[CH_COUNT];
    SPI_HandleTypeDef* hspi;
} ChannelController;

void initChannelController(ChannelController* chCtrl, SPI_HandleTypeDef* hspi);

void applyChannelsConfiguration(ChannelController* chCtrl);

void initChannelFromGPIO_(Channel* ch, GPIO_TypeDef* gpioPort, uint32_t gpioPin);

void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer);

void applyChannelConfiguration(Channel* ch);

void applyTimerChannelConfig_(Channel* ch);

void applyGPIOChannelConfig_(Channel* ch);

void pushConfigSignalsToShiftRegister_(Channel* ch, uint32_t* shiftRegisterValues);

Channel* getChannelFromNumber(uint32_t channelNumber);

#endif // CHANNEL_CONTROLLER_h