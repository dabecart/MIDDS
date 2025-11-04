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

void initChannelController(ChannelController* chCtrl, SPI_HandleTypeDef* hspi, 
    I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c2) {
    if(chCtrl == NULL || hspi == NULL) return;
    
    chCtrl->hspi = hspi;
    
    initGPIOExpander(&chCtrl->gExp5V,  hi2c1, CH_GPIO_EXP_5V_ADDRS);
    initGPIOExpander(&chCtrl->gExp3V3, hi2c1, CH_GPIO_EXP_3V3_ADDRS);
    initGPIOExpander(&chCtrl->gExp1V8, hi2c2, CH_GPIO_EXP_1V8_ADDRS);

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

    // In the PCB: GPIO 00 from the GPIO expander corresponds to channel 31.
    int reverseCounter = 15;
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);
    initChannelFromGPIO_(&chCtrl->channels[channelCount++], reverseCounter--);

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

void initChannelFromGPIO_(Channel* ch, uint16_t gpioExpPin) {
    ch->type = CHANNEL_GPIO;
    ch->mode = CHANNEL_DISABLED;
    ch->protocol = CHANNEL_PROTOC_OFF;
    
    GPIOChannel* gpio = &ch->data.gpio;
    gpio->pinNumber = gpioExpPin;
}

void initChannelFromHWTimer_(Channel* ch, HWTimerChannel* hwTimer) {
    ch->type = CHANNEL_TIMER;
    ch->mode = CHANNEL_DISABLED;
    ch->protocol = CHANNEL_PROTOC_OFF;
    
    TimerChannel* timCh = &ch->data.timer;
    timCh->timerHandler = hwTimer;
    setHWTimerEnabled(hwTimer, 0);
}

void applyChannelConfiguration(Channel* ch) {
    if(ch->type == CHANNEL_TIMER) {
        applyTimerChannelConfig_(ch);
        // Erase the buffers.
        empty_cb64(&ch->data.timer.timerHandler->data);
    }else if(ch->type == CHANNEL_GPIO) {
        applyGPIOChannelConfig_(ch);
    }else {
        // TODO: Error handler.
    }
}

void applyTimerChannelConfig_(Channel* ch) {
    if(ch->type != CHANNEL_TIMER) return;

    HWTimerChannel* timCh = ch->data.timer.timerHandler;

    // Always deinit first.
    setHWTimerEnabled(timCh, 0);
    
    // These lines are inside HAL_TIM_IC_Stop_IT but they do not disable the whole timer.
    TIM_CCxChannelCmd(timCh->htim->Instance, timCh->timChannel, TIM_CCx_DISABLE);
    TIM_CHANNEL_STATE_SET(timCh->htim, timCh->timChannel, HAL_TIM_CHANNEL_STATE_READY);
    TIM_CHANNEL_N_STATE_SET(timCh->htim, timCh->timChannel, HAL_TIM_CHANNEL_STATE_READY);

    HAL_GPIO_DeInit(timCh->gpioPort, timCh->gpioPin);

    if(ch->mode == CHANNEL_DISABLED) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(ch->mode == CHANNEL_OUTPUT){
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Pin = timCh->gpioPin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    
        HAL_GPIO_Init(timCh->gpioPort, &GPIO_InitStruct);
    }else{
        // Taken from stm32g4xx_hal_msp.c.
        GPIO_InitStruct.Pin = timCh->gpioPin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

        if(timCh->htim->Instance==TIM1)         GPIO_InitStruct.Alternate = GPIO_AF6_TIM1;
        else if(timCh->htim->Instance==TIM2)    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        else if(timCh->htim->Instance==TIM3)    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        else if(timCh->htim->Instance==TIM4)    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
        else                                    return;
        
        HAL_GPIO_Init(timCh->gpioPort, &GPIO_InitStruct);

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

        HAL_TIM_IC_Start_IT(timCh->htim, timCh->timChannel);
        // Enable the interruptions on the channel.
        setHWTimerEnabled(timCh, 1);
    }
}

void applyGPIOChannelConfig_(Channel* ch) {
    if(ch->type != CHANNEL_GPIO) return;

    // First, configure the pin as input in all three expanders.
    setDirectionGPIOExpander(&chCtrl.gExp5V, ch->data.gpio.pinNumber, GPIOEx_Input);
    setDirectionGPIOExpander(&chCtrl.gExp3V3, ch->data.gpio.pinNumber, GPIOEx_Input);
    setDirectionGPIOExpander(&chCtrl.gExp1V8, ch->data.gpio.pinNumber, GPIOEx_Input);

    // Then, if the channel is an output, configure it on the corresponding GPIO expander.
    if(ch->mode == CHANNEL_OUTPUT) {
        GPIOExpander* exp = getGPIOExpanderFromGPIOChannel_(ch);
        if(exp == NULL) return;

        // Initiate as low.
        setStateGPIOExpander(exp, ch->data.gpio.pinNumber, GPIOEx_LOW);
        setDirectionGPIOExpander(exp, ch->data.gpio.pinNumber, GPIOEx_Output);
    }
}

void setShiftRegisterValues(ChannelController* channelController) {
    // Sends the values stored on the configuration of the channels to the Shift Registers that 
    // control the direction of the GPIOs. There's one shift register per channel.
    TimerChannel_ShiftReg srDataOutBuf[CH_CONT_TIMER_COUNT] = {0};
    TimerChannel_ShiftReg* stDataOut;
    Channel* ch;
    for(int16_t i = CH_CONT_TIMER_COUNT-1; i >= 0; i--) {
        ch = &channelController->channels[i];
        stDataOut = &srDataOutBuf[CH_CONT_TIMER_COUNT-1-i];
        
        // Voltage signals.
        stDataOut->v1 = (ch->protocol == CHANNEL_PROTOC_5V) || (ch->protocol == CHANNEL_PROTOC_1V8) || 
                        (ch->protocol == CHANNEL_PROTOC_LVDS);
        stDataOut->v2 = (ch->protocol == CHANNEL_PROTOC_3V3) || (ch->protocol == CHANNEL_PROTOC_1V8);

        // Status LEDs.
        stDataOut->statusGreen = (ch->mode != CHANNEL_DISABLED);
        stDataOut->statusRed = (ch->mode == CHANNEL_DISABLED);

        // RE/DE signals.
        stDataOut->re = 
            ((ch->protocol != CHANNEL_PROTOC_LVDS) && (ch->mode != CHANNEL_DISABLED)) || 
            ((ch->protocol == CHANNEL_PROTOC_LVDS) && (ch->mode == CHANNEL_OUTPUT));

        stDataOut->de = (ch->protocol == CHANNEL_PROTOC_LVDS);

        // To set the DIR pin.
        stDataOut->isOut = (ch->mode == CHANNEL_OUTPUT);
    }

    HAL_SPI_Transmit(
        channelController->hspi, (uint8_t*) (&srDataOutBuf), sizeof(srDataOutBuf), 1000);

    // Make the SR output its inner content by toggling the ENABLE pin. 
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET);
    // This behemoth is a 50ns delay. It gives some time to the shift register to "register" the 
    // pulse. Nevertheless, if there are interrupts in the middle, this time will be longer, but 
    // that's actually OK.
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_SET); 
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    HAL_GPIO_WritePin(SHIFT_REG_ENABLE_GPIO_Port, SHIFT_REG_ENABLE_Pin, GPIO_PIN_RESET); 
}

Channel* getChannelFromNumber(uint32_t channelNumber) {
    if(channelNumber >= CH_COUNT) return NULL;
    return &chCtrl.channels[channelNumber];
}

GPIOExpander* getGPIOExpanderFromGPIOChannel_(Channel* ch) {
    if((ch == NULL) || (ch->type != CHANNEL_GPIO)) return NULL;

    switch (ch->protocol) {
        case CHANNEL_PROTOC_5V:     return &chCtrl.gExp5V;
    
        case CHANNEL_PROTOC_3V3:    return &chCtrl.gExp3V3;

        case CHANNEL_PROTOC_1V8:    return &chCtrl.gExp1V8;

        default: return NULL;
    }
}

uint8_t setChannelState(Channel* ch, uint8_t newState) {
    if((ch == NULL) || (ch->mode != CHANNEL_OUTPUT)) return 0;

    if(ch->type == CHANNEL_TIMER) {
        TimerChannel* timerCh = &ch->data.timer;
        HAL_GPIO_WritePin(timerCh->timerHandler->gpioPort, 
                          timerCh->timerHandler->gpioPin, 
                          newState ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }else if(ch->type == CHANNEL_GPIO) {
        return setStateGPIOExpander(getGPIOExpanderFromGPIOChannel_(ch),
                                    ch->data.gpio.pinNumber,
                                    newState ? GPIOEx_HIGH : GPIOEx_LOW);
    }else {
        return 0;
    }

    return 1;
}

uint8_t getChannelState(Channel* ch, uint8_t* currentState) {
    if(ch == NULL) return 0;

    if(ch->type == CHANNEL_TIMER) {
        TimerChannel* timerCh = &ch->data.timer;
        GPIO_PinState state = HAL_GPIO_ReadPin(timerCh->timerHandler->gpioPort, 
                                               timerCh->timerHandler->gpioPin);
        *currentState = (state == GPIO_PIN_SET);
    }else if(ch->type == CHANNEL_GPIO) {
        GPIOEx_State state;
        if(!getStateGPIOExpander(getGPIOExpanderFromGPIOChannel_(ch),
                                 ch->data.gpio.pinNumber,
                                 &state)) {
            return 0;
        }
        *currentState = (state == GPIOEx_HIGH);
    }else {
        return 0;
    }

    return 1;
}
