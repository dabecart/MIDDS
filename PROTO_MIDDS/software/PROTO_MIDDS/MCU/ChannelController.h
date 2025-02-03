#ifndef CHANNEL_CONTROLLER_h
#define CHANNEL_CONTROLLER_h

#include "stm32g4xx_hal.h"
#include "HWTimers.h"

#define CH_CONT_TIMER_COUNT     14
#define CH_CONT_GPIO_COUNT      16

typedef enum ChannelMode {
    CHANNEL_OFF     = 0,
    CHANNEL_INPUT   = 1,
    CHANNEL_OUTPUT  = 2
} ChannelMode;

typedef enum ChannelProtocol {
    CHANNEL_TTL   = 1,
    CHANNEL_LVDS  = 2
} ChannelProtocol;

typedef enum ChannelEdges {
    CHANNEL_RISING          = 1,
    CHANNEL_FALLING         = 2,
    CHANNEL_RISING_FALLING  = 3
} ChannelEdges;

typedef struct Channel {
    HWTimerChannel*     timerHandler;
    ChannelMode         mode;
    ChannelProtocol     protocol;
    ChannelEdges        edges;
} Channel;

typedef struct GPIO {
    GPIO_TypeDef*       gpioPort;
    uint32_t            gpioPin;
    ChannelMode         mode;
} GPIO;

typedef struct ChannelController {
    Channel channels[CH_CONT_TIMER_COUNT];
    GPIO gpios[CH_CONT_GPIO_COUNT];
    
    SPI_HandleTypeDef* hspi;
} ChannelController;

void initChannelController(ChannelController* chController, SPI_HandleTypeDef* hspi);

void applyChannelsConfiguration(ChannelController* chController);

void initChannelFromGPIO_(GPIO* gpio, GPIO_TypeDef* gpioPort, uint32_t gpioPin);

void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer);

void pushConfigSignalsToShiftRegister_(Channel* ch, uint32_t* shiftRegisterValues);

#endif // CHANNEL_CONTROLLER_h