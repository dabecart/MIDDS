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

#include "ChannelController.h"
#include "MainMCU.h"

void initChannelController(ChannelController* chCtrl, SPI_HandleTypeDef* hspi) {
    if(chCtrl == NULL || hspi == NULL) return;
    
    chCtrl->hspi = hspi;

    // Set the Shift Register Enable off.
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 

    // Initialize channels.
    // First channels are timer related and the rest are GPIO based.
    int channelCount;
    for(channelCount = 0; channelCount < CH_CONT_TIMER_COUNT; channelCount++) {
        initChannelFromHWTimer_(
            &chCtrl->channels[channelCount],  
            &hwTimers.channels[channelCount]
        );
    }

    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO00_GPIO_Port, GPIO00_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO01_GPIO_Port, GPIO01_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO02_GPIO_Port, GPIO02_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO03_GPIO_Port, GPIO03_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO04_GPIO_Port, GPIO04_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO05_GPIO_Port, GPIO05_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO06_GPIO_Port, GPIO06_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO07_GPIO_Port, GPIO07_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO08_GPIO_Port, GPIO08_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO09_GPIO_Port, GPIO09_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO10_GPIO_Port, GPIO10_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO11_GPIO_Port, GPIO11_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO12_GPIO_Port, GPIO12_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO13_GPIO_Port, GPIO13_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO14_GPIO_Port, GPIO14_Pin);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], GPIO15_GPIO_Port, GPIO15_Pin);

    // Disable all HWTimers.
    for(int i = 0; i < CH_COUNT; i++) {
        if(chCtrl->channels[i].type == CHANNEL_TIMER) {
            setHWTimerEnabled(chCtrl->channels[i].data.timer.timerHandler, 0);
        }
    }

    // Apply the configuration to all individual channels.
    for(int i = 0; i < CH_COUNT; i++) {
        applyChannelConfiguration(chCtrl->channels + i);
    }
    setShiftRegisterValues(chCtrl);

    // Reenable timers.
    for(int i = 0; i < CH_COUNT; i++) {
        if(chCtrl->channels[i].type == CHANNEL_TIMER) {
            setHWTimerEnabled(chCtrl->channels[i].data.timer.timerHandler, 
                                (chCtrl->channels[i].mode != CHANNEL_DISABLED));
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
    timCh->signalType = CHANNEL_SIGNAL_TTL;
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

    HWTimerChannel* timCh = ch->data.timer.timerHandler;

    if(ch->mode == CHANNEL_DISABLED) {
        setHWTimerEnabled(timCh, 0);
        return;
    }

    // Set the mode of the HW Timers.
    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;

    if(ch->mode == CHANNEL_MONITOR_RISING_EDGES) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    }else if(ch->mode == CHANNEL_MONITOR_FALLING_EDGES) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    }else if((ch->mode == CHANNEL_MONITOR_BOTH_EDGES) || 
             (ch->mode == CHANNEL_INPUT) || 
             (ch->mode == CHANNEL_FREQUENCY)) {
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
    }

    if (HAL_TIM_IC_ConfigChannel(timCh->htim,
                                 &sConfigIC, 
                                 timCh->timChannel) != HAL_OK) {
        // TODO: Handle this...
    }

    // Enable the interruptions on the channel.
    setHWTimerEnabled(timCh, 1);
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

void setShiftRegisterValues(ChannelController* chCtrl) {
    // Sends the values stored on the configuration of the channels to the Shift Registers that 
    // control the direction of the GPIOs. They are four SR, therefore, 32 bits to generate.
    uint32_t shiftRegisterDataOut = 0;
    Channel* ch;
    TimerChannel* timCh;
    uint8_t RE, DE;
    for(int i = 0; i < CH_CONT_TIMER_COUNT; i++) {
        ch = &chCtrl->channels[i];
        timCh = &ch->data.timer;
        
        RE = ((timCh->signalType == CHANNEL_SIGNAL_TTL) && (ch->mode == CHANNEL_INPUT)) ||
             ((timCh->signalType == CHANNEL_SIGNAL_LVDS) && (ch->mode == CHANNEL_OUTPUT));
        
        DE = (timCh->signalType == CHANNEL_SIGNAL_LVDS);
        
        shiftRegisterDataOut <<= 2;
        shiftRegisterDataOut |= (RE << 1) | DE;
    }

    // shiftRegisterDataOut holds the data as:
    // MSB  -------------------- LSB
    // RE0 DE0 RE1 DE1 ... RE13 DE13
    // Following the circuit, the shift registers are connected as:
    // SR0 > SR1 > SR2 > SR3
    // On the SR1 and SR3, the G and H channels are not used, that is:
    // - Channels  0 to  3 are on SR0.
    // - Channels  4 to  6 are on SR1.
    // - Channels  7 to 10 are on SR2.
    // - Channels 11 to 13 are on SR3.
    // Bits must be shifted to put "zeros" on those G and H ports.
    shiftRegisterDataOut = ((shiftRegisterDataOut & 0xFFFC000) << 4) | ((shiftRegisterDataOut & 0x3FFF) << 2);

    HAL_SPI_Transmit(
        chCtrl->hspi, (uint8_t*) (&shiftRegisterDataOut), sizeof(uint32_t), 1000);

    // Make the SR output its inner content by toggling the ENABLE pin. 
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 
    HAL_Delay(1);
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_SET); 
    HAL_Delay(1);
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 
}

Channel* getChannelFromNumber(uint32_t channelNumber) {
    if(channelNumber >= CH_COUNT) return NULL;
    return &chCtrl.channels[channelNumber];
}
