#include "ChannelController.h"
#include "MainMCU.h"

void initChannelController(ChannelController* chController, SPI_HandleTypeDef* hspi) {
    if(chController == NULL || hspi == NULL) return;
    
    chController->hspi = hspi;

    // Set the Shift Register Enable off.
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 

    // Initialize single channels.
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        initChannelFromHWTimer_(&chController->channels[i],  &hwTimers.channels[i]);
    }

    int gpioCount = 0;
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO00_GPIO_Port, GPIO00_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO01_GPIO_Port, GPIO01_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO02_GPIO_Port, GPIO02_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO03_GPIO_Port, GPIO03_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO04_GPIO_Port, GPIO04_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO05_GPIO_Port, GPIO05_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO06_GPIO_Port, GPIO06_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO07_GPIO_Port, GPIO07_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO08_GPIO_Port, GPIO08_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO09_GPIO_Port, GPIO09_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO10_GPIO_Port, GPIO10_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO11_GPIO_Port, GPIO11_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO12_GPIO_Port, GPIO12_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO13_GPIO_Port, GPIO13_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO14_GPIO_Port, GPIO14_Pin);
    initChannelFromGPIO_(&chController->gpios[gpioCount++], GPIO15_GPIO_Port, GPIO15_Pin);

    applyChannelsConfiguration(chController);

}

void applyChannelsConfiguration(ChannelController* chController) {
    // Disable all HWTimers.
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        setHWTimerEnabled(chController->channels[i].timerHandler, 0);
    }

    // Set the mode of the HW Timers.
    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;

    Channel* channel;
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        channel = &chController->channels[i];

        if(channel->mode == CHANNEL_INPUT) {
            switch (channel->edges) {
                case CHANNEL_RISING:
                    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
                    break;
                case CHANNEL_FALLING:
                    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
                    break;
                case CHANNEL_RISING_FALLING:
                    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
                    break;
                default:
                    // TODO: Handle this...
                    break;
            }
            
            if (HAL_TIM_IC_ConfigChannel(channel->timerHandler->htim,
                                        &sConfigIC, 
                                        channel->timerHandler->timChannel) != HAL_OK) {
                // TODO: Handle this...
            }
        }
    }

    // Set the GPIO configuration of the MCU.
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO* gpio;
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        gpio = &chController->gpios[i];
        if(gpio->mode == CHANNEL_OFF) {
            HAL_GPIO_DeInit(gpio->gpioPort, gpio->gpioPin);
        }else {
            GPIO_InitStruct.Pin = gpio->gpioPin;
            GPIO_InitStruct.Mode = (gpio->mode == CHANNEL_OUTPUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
            HAL_GPIO_Init(gpio->gpioPort, &GPIO_InitStruct);
        }
    }
    
    // Sends the values stored on the configuration of the channels to the Shift Registers that 
    // control the direction of the GPIOs. They are four SR, therefore, 32 bits to generate.
    uint32_t shiftRegisterDataOut = 0;
    for(int i = CH_CONT_TIMER_COUNT-1; i >= 0; i--) {
        pushConfigSignalsToShiftRegister_(&chController->channels[i], &shiftRegisterDataOut);
    }
    HAL_SPI_Transmit(chController->hspi, (uint8_t*) shiftRegisterDataOut, sizeof(uint32_t), 1000);

    // Reenable timers.
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        setHWTimerEnabled(chController->channels[i].timerHandler, 
                          chController->channels[i].mode != CHANNEL_OFF);
    }

}

void initChannelFromGPIO_(GPIO* gpio, GPIO_TypeDef* gpioPort, uint32_t gpioPin) {
    gpio->gpioPort = gpioPort;
    gpio->gpioPin = gpioPin;
    gpio->mode = CHANNEL_OFF;
}

void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer) {
    ch->timerHandler = hwTimer;
    setHWTimerEnabled(hwTimer, 0);

    ch->mode = CHANNEL_OFF;
    ch->protocol = CHANNEL_TTL;
    ch->edges = CHANNEL_RISING;
}

void pushConfigSignalsToShiftRegister_(Channel* ch, uint32_t* shiftRegisterValues) {
    uint8_t RE = (ch->protocol == CHANNEL_TTL) | (ch->protocol == CHANNEL_LVDS && ch->mode == CHANNEL_OUTPUT);
    uint8_t DE = (ch->protocol == CHANNEL_LVDS);
    *shiftRegisterValues <<= 2;
    *shiftRegisterValues |= (RE << 1) | DE;
}