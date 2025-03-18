#include "ChannelController.h"
#include "MainMCU.h"

void initChannelController(ChannelController* chController, SPI_HandleTypeDef* hspi) {
    if(chController == NULL || hspi == NULL) return;
    
    chController->hspi = hspi;

    // Set the Shift Register Enable off.
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 

    // Initialize single channels.
    int channelCount;
    for(channelCount = 0; channelCount < CH_CONT_TIMER_COUNT; channelCount++) {
        initChannelFromHWTimer_(
            &chController->channels[channelCount],  
            &hwTimers.channels[channelCount]
        );
    }

    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO00_GPIO_Port, GPIO00_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO01_GPIO_Port, GPIO01_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO02_GPIO_Port, GPIO02_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO03_GPIO_Port, GPIO03_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO04_GPIO_Port, GPIO04_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO05_GPIO_Port, GPIO05_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO06_GPIO_Port, GPIO06_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO07_GPIO_Port, GPIO07_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO08_GPIO_Port, GPIO08_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO09_GPIO_Port, GPIO09_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO10_GPIO_Port, GPIO10_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO11_GPIO_Port, GPIO11_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO12_GPIO_Port, GPIO12_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO13_GPIO_Port, GPIO13_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO14_GPIO_Port, GPIO14_Pin);
    initChannelFromGPIO_(&chController->channels[channelCount++], GPIO15_GPIO_Port, GPIO15_Pin);

    applyChannelsConfiguration(chController);
}

void applyChannelsConfiguration(ChannelController* chController) {
    // Disable all HWTimers.
    for(int i = 0; i < CH_COUNT; i++) {
        if(chController->channels[i].type == CHANNEL_TIMER) {
            setHWTimerEnabled(chController->channels[i].data.timer.timerHandler, 0);
        }
    }

    // Sends the values stored on the configuration of the channels to the Shift Registers that 
    // control the direction of the GPIOs. They are four SR, therefore, 32 bits to generate.
    uint32_t shiftRegisterDataOut = 0;
    for(int i = CH_CONT_TIMER_COUNT-1; i >= 0; i--) {
        pushConfigSignalsToShiftRegister_(&chController->channels[i], &shiftRegisterDataOut);
    }
    HAL_SPI_Transmit(chController->hspi, (uint8_t*) (&shiftRegisterDataOut), sizeof(uint32_t), 1000);

    // Reenable timers.
    for(int i = 0; i < CH_COUNT; i++) {
        if(chController->channels[i].type == CHANNEL_TIMER) {
            setHWTimerEnabled(chController->channels[i].data.timer.timerHandler, 
                              (chController->channels[i].mode != CHANNEL_DISABLED));
        }
    }

}

void initChannelFromGPIO_(Channel* ch, GPIO_TypeDef* gpioPort, uint32_t gpioPin) {
    ch->type = CHANNEL_GPIO;
    ch->mode = CHANNEL_DISABLED;
    
    GPIOChannel* gpio = &ch->data.gpio;
    gpio->gpioPort = gpioPort;
    gpio->gpioPin = gpioPin;
}

void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer) {
    ch->type = CHANNEL_TIMER;
    ch->mode = CHANNEL_DISABLED;
    
    TimerChannel* timCh = &ch->data.timer;
    timCh->timerHandler = hwTimer;
    timCh->protocol = CHANNEL_SIGNAL_TTL;
    setHWTimerEnabled(hwTimer, 0);
}

void applyChannelConfiguration(Channel* ch) {
    if(ch->type == CHANNEL_TIMER) {
        applyTimerChannelConfig_(ch);
    }else if(ch->type == CHANNEL_GPIO) {
        applyGPIOChannelConfig_(ch);
    }else {
        // TODO: Error handler.
    }
}

void applyTimerChannelConfig_(Channel* ch) {
    if(ch->type != CHANNEL_TIMER) return;

    // Set the mode of the HW Timers.
    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;

    TimerChannel* timCh = &ch->data.timer;
    if(ch->mode == CHANNEL_MONITOR_RISING_EDGES) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    }else if(ch->mode == CHANNEL_MONITOR_FALLING_EDGES) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    }else if(ch->mode == CHANNEL_MONITOR_BOTH_EDGES) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
    }

    if (HAL_TIM_IC_ConfigChannel(timCh->timerHandler->htim,
                                 &sConfigIC, 
                                 timCh->timerHandler->timChannel) != HAL_OK) {
        // TODO: Handle this...
    }
}

void applyGPIOChannelConfig_(Channel* ch) {
    if(ch->type != CHANNEL_GPIO) return;

    // Set the GPIO configuration of the MCU.
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIOChannel* gpio = &ch->data.gpio;
    if(ch->mode == CHANNEL_DISABLED) {
        HAL_GPIO_DeInit(gpio->gpioPort, gpio->gpioPin);
    }else {
        GPIO_InitStruct.Pin = gpio->gpioPin;
        GPIO_InitStruct.Mode = (ch->mode == CHANNEL_OUTPUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
        HAL_GPIO_Init(gpio->gpioPort, &GPIO_InitStruct);
    }
}

void pushConfigSignalsToShiftRegister_(Channel* ch, uint32_t* shiftRegisterValues) {
    if(ch->type != CHANNEL_TIMER) {
        return;
    }

    TimerChannel* timCh = &ch->data.timer;
    uint8_t RE = ((timCh->protocol == CHANNEL_SIGNAL_TTL) && (ch->mode == CHANNEL_INPUT)) ||
                 ((timCh->protocol == CHANNEL_SIGNAL_LVDS) && (ch->mode == CHANNEL_OUTPUT));
    
    uint8_t DE = (timCh->protocol == CHANNEL_SIGNAL_LVDS);
    
    *shiftRegisterValues <<= 2;
    *shiftRegisterValues |= (RE << 1) | DE;
}
