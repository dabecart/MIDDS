/***************************************************************************************************
 * @file Comms.c
 * @brief Implementation of the communication protocol of the MIDDS.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2025-07-29
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "Comms.h"
#include "MainMCU.h"

// This buffer gets filled inside "usbd_cdc_if.c"'s "CDC_Receive_FS" function.
CircularBuffer inputBuffer;
CircularBuffer outputBuffer;

extern USBD_HandleTypeDef hUsbDeviceFS;

// Set to 1 when the USB is connected.
uint8_t usbConnected = 0;

void initComms() {
    init_cb(&inputBuffer, CIRCULAR_BUFFER_MAX_SIZE);
    init_cb(&outputBuffer, CIRCULAR_BUFFER_MAX_SIZE);
}

void receiveData() {
    static uint8_t inMsgBuffer[COMMS_MAX_MSG_INPUT_LEN];
    uint8_t peekChar;

    while(inputBuffer.len > 0) {
        peek_cb(&inputBuffer, &peekChar);
        if(peekChar == COMMS_MSG_SYNC) {
            // It's important to save the length of the input buffer as inputBuffer will be
            // modified by the ISR raised by the USB. If the ISR is raised just after 
            // entering the if statement below, it will write to inMsgBuffer much more than
            // it can handle, causing a memory overflow. 
        	uint32_t inputLength = inputBuffer.len;
            if(inputLength > COMMS_MAX_MSG_INPUT_LEN) {
                peekN_cb(&inputBuffer, COMMS_MAX_MSG_INPUT_LEN, inMsgBuffer);
            }else {
                peekN_cb(&inputBuffer, inputLength, inMsgBuffer);
            }
            
            int32_t decodeReturn = decodeGPIOMessage(inMsgBuffer, inputLength);
            if(decodeReturn == COMMS_DECODE_NOT_ENOUGH_DATA) {
                // Do not discard any data. Exit the function and come back later.
                return;
            }else if((decodeReturn == COMMS_DECODE_ERROR_DECODING) || 
                     (decodeReturn == COMMS_DECODE_SYNC_SEQUENCE_NOK)) {
                // The message was not well structured. Pop the starting character and continue.
                pop_cb(&inputBuffer, &peekChar);
            }else if(decodeReturn >= 0) {
                // The message was well structured and it was (or was not) well executed.
                popN_cb(&inputBuffer, decodeReturn, inMsgBuffer);
            }else {
                // This part should not be entered...
                pop_cb(&inputBuffer, &peekChar);
            }
        }else {
            // If the initial char is not the COMMS_MSG_SYNC, pop it and continue searching.
            pop_cb(&inputBuffer, &peekChar);
        }
    }
}

void sendData() {
    static uint32_t outMsgLen = 0;

    if(!usbConnected) {
        outMsgLen = 0;
        empty_cb(&inputBuffer);
        empty_cb(&outputBuffer);
    }

    // If there's no pending message to send (outMsgLen == 0) and there's something saved to send
    // (outputBuffer.len > 0) calculate the number of bytes to send.
    if((outMsgLen == 0) && (outputBuffer.len > 0)) {
        outMsgLen = outputBuffer.len;
        if(outMsgLen > sizeof(UserTxBufferFS)) outMsgLen = sizeof(UserTxBufferFS);
        if(!popN_cb(&outputBuffer, outMsgLen, UserTxBufferFS)) outMsgLen = 0;
    }

    // CDC_Transmit_FS really transmits the message when it return USBD_OK. 
    if((outMsgLen != 0) && (CDC_Transmit_FS(UserTxBufferFS, outMsgLen) == USBD_OK)) {
        // When transmitted, set the output message length back to zero. 
        outMsgLen = 0;
    }
}

uint8_t encodeGPIOMessage(const ChannelMessageType msgType, const ChannelMessage msg) {
    static uint8_t outMsgBuffer[CIRCULAR_BUFFER_MAX_SIZE]; 
    uint32_t maxLength = outputBuffer.size - outputBuffer.len;
    uint32_t messageLen = 0;
    
    switch (msgType) {
        case GPIO_MSG_INPUT: {
            messageLen = encodeInput(&msg.input, outMsgBuffer);
            break;
        }

        case GPIO_MSG_FREQUENCY: {
            messageLen = encodeFrequency(&msg.frequency, outMsgBuffer);
            break;
        }

        case GPIO_MSG_MONITOR: {
            messageLen = encodeMonitor(msg.monitor, outMsgBuffer, maxLength);
            break;
        }

        case GPIO_MSG_ERROR: {
            messageLen = encodeError(&msg.error, outMsgBuffer, maxLength);
            break;
        }

        default: {
            // TODO: Invalid message type.
            return 0;
        }
    }

    // Push to the output buffer.
    if((messageLen == 0) || (messageLen > maxLength) || 
       !pushN_cb(&outputBuffer, outMsgBuffer, messageLen)) {
        return 0;
    }
    return 1;
}

int32_t decodeGPIOMessage(const uint8_t* dataBuffer, const uint32_t dataLen) {
    if(dataLen < COMMS_MIN_MSG_LEN) {
        return COMMS_DECODE_NOT_ENOUGH_DATA;
    }

    if(dataBuffer[0] != COMMS_MSG_SYNC) {
        return COMMS_DECODE_SYNC_SEQUENCE_NOK;
    }

    const char* messageID = (char*) (dataBuffer + 1);
    // Set to 1 just in case something doesn't get caught. At least something will be deleted.
    int32_t messageLen = 1;

    if(strncmp(messageID, COMMS_MSG_INPUT_HEAD, strlen(COMMS_MSG_INPUT_HEAD)) == 0) {
        ChannelInput temp = {};
        if(dataLen < COMMS_MSG_INPUT_LEN)               return COMMS_DECODE_NOT_ENOUGH_DATA;
        if(!decodeInput(dataBuffer, &temp))             return COMMS_DECODE_ERROR_DECODING;
        
        messageLen = COMMS_MSG_INPUT_LEN;
        executeInputCommand(&temp);
    }else if(strncmp(messageID, COMMS_MSG_OUTPUT_HEAD, strlen(COMMS_MSG_OUTPUT_HEAD)) == 0) {
        ChannelOutput temp = {};
        if(dataLen < COMMS_MSG_OUTPUT_LEN)              return COMMS_DECODE_NOT_ENOUGH_DATA;
        if(!decodeOutput(dataBuffer, &temp))            return COMMS_DECODE_ERROR_DECODING;
        
        messageLen = COMMS_MSG_OUTPUT_LEN;
        executeOutputCommand(&temp);
    }else if(strncmp(messageID, COMMS_MSG_FREQ_HEAD, strlen(COMMS_MSG_FREQ_HEAD)) == 0) {
        ChannelFrequency temp = {};
        if(dataLen < COMMS_MSG_FREQ_LEN)                return COMMS_DECODE_NOT_ENOUGH_DATA;
        if(!decodeFrequency(dataBuffer, &temp))         return COMMS_DECODE_ERROR_DECODING;
        
        messageLen = COMMS_MSG_FREQ_LEN;
        executeFrequencyCommand(&temp);
    }else if(strncmp(messageID, COMMS_MSG_CHANNEL_SETT_HEAD, strlen(COMMS_MSG_CHANNEL_SETT_HEAD)) == 0) {
        ChannelSettingsChannel temp = {};
        if(dataLen < COMMS_MSG_CHANNEL_SETT_LEN)        return COMMS_DECODE_NOT_ENOUGH_DATA;
        if(!decodeSettingsChannel(dataBuffer, &temp))   return COMMS_DECODE_ERROR_DECODING;

        messageLen = COMMS_MSG_CHANNEL_SETT_LEN;
        executeChannelSettingsCommand(&temp);
    }else if(strncmp(messageID, COMMS_MSG_SYNC_SETT_HEAD, strlen(COMMS_MSG_SYNC_SETT_HEAD)) == 0) {
        ChannelSettingsSYNC temp = {};
        if(dataLen < COMMS_MSG_SYNC_SETT_LEN)           return COMMS_DECODE_NOT_ENOUGH_DATA;
        if(!decodeSettingsSync(dataBuffer, &temp))      return COMMS_DECODE_ERROR_DECODING;

        messageLen = COMMS_MSG_SYNC_SETT_LEN;
        executeSyncSettingsCommand(&temp);
    }else if(strncmp(messageID, COMMS_MSG_CONNECT_HEAD, strlen(COMMS_MSG_CONNECT_HEAD)) == 0) {
        messageLen = COMMS_MSG_CONN_LEN;
        establishConnection(1);
    }else if(strncmp(messageID, COMMS_MSG_DISCONNECT_HEAD, strlen(COMMS_MSG_DISCONNECT_HEAD)) == 0) {
        messageLen = COMMS_MSG_DISC_LEN;
        establishConnection(0);
    }else {
        return COMMS_DECODE_SYNC_SEQUENCE_NOK;
    }

    return messageLen;
}

uint16_t encodeInput(const ChannelInput* dataStruct, uint8_t* outBuffer) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;

    uint16_t len = sprintf((char*) outBuffer, 
                    "%c%s%02ld%c",
                    COMMS_MSG_SYNC, COMMS_MSG_INPUT_HEAD,
                    dataStruct->channel, dataStruct->value);
    memcpy(outBuffer + len, (uint8_t*) &dataStruct->time, sizeof(dataStruct->time));
    return len + sizeof(dataStruct->time);
}

uint16_t encodeMonitor(HWTimerChannel* hwTimer, uint8_t* outBuffer, const uint16_t maxMsgLen){
    if((hwTimer == NULL) || (outBuffer == NULL) || 
       (hwTimer->data.len == 0) || (maxMsgLen < COMMS_MIN_MONITOR_MSG_LEN)){
        return 0;
    } 

    uint16_t messageCount = hwTimer->data.len;   // Make it constant at this point.
    if(messageCount > COMMS_MAX_TIMESTAMPS_IN_MONITOR) {
        messageCount = COMMS_MAX_TIMESTAMPS_IN_MONITOR;
    }

    uint16_t maxMessageCount = (maxMsgLen - COMMS_MSG_MONITOR_HEADER_LEN)/COMMS_MONITOR_TIMESTAMP_LEN;
    if(messageCount > maxMessageCount) {
        messageCount = maxMessageCount;
    }
    
    // To make the pop_cb64, this header must be a multiple of eight bytes long (in binary output 
    // mode at least).
    uint16_t msgSize = COMMS_MSG_MONITOR_HEADER_LEN;
    sprintf((char*) outBuffer, "%c%s%02d%04d", 
            COMMS_MSG_SYNC, COMMS_MSG_MONITOR_HEAD, hwTimer->channelNumber, messageCount);

#if MCU_TX_IN_ASCII
    uint64_t readVal;
    for(uint8_t countIndex = 0; countIndex < messageCount; countIndex++) {
        pop_cb64(&hwTimer->data, &readVal);
        msgSize += snprintf64Hex(
                        outBuffer + msgSize, 
                        maxMsgLen - msgSize,
                        readVal
                    );
        if((maxMsgLen - msgSize) <= 16) {
            break;
        }
    }
    msgSize += snprintf(outBuffer + msgSize, maxMsgLen-msgSize, "\n");
#else
    uint64_t readVal;
    for(uint32_t countIndex = 0; countIndex < messageCount; countIndex++) {
        pop_cb64(&hwTimer->data, &readVal);

        // The LSB is the value of the channel (HIGH or LOW). The rest is the timestamp in internal 
        // time. Convert to UNIX time the timestamp, shift it to the left one bit and add the value
        // of the channel.
        readVal = (convertFromInternalToUNIXTime(readVal >> 1) << 1) | (readVal & 0x01ULL);

        memcpy(outBuffer + msgSize, &readVal, COMMS_MONITOR_TIMESTAMP_LEN);
        msgSize += COMMS_MONITOR_TIMESTAMP_LEN;
    }
#endif

    hwTimer->lastPrintTick = HAL_GetTick();
    return msgSize;
}

uint16_t encodeFrequency(const ChannelFrequency* dataStruct, uint8_t* outBuffer) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;
    uint16_t len = sprintf((char*) outBuffer, 
                            "%c%s%02ld", 
                            COMMS_MSG_SYNC, COMMS_MSG_FREQ_HEAD,
                            dataStruct->channel);
    memcpy(outBuffer + len, &dataStruct->frequency, sizeof(dataStruct->frequency));
    len += sizeof(dataStruct->frequency);

    memcpy(outBuffer + len, &dataStruct->dutyCycle, sizeof(dataStruct->dutyCycle));
    len += sizeof(dataStruct->dutyCycle);
    
    memcpy(outBuffer + len, &dataStruct->time, sizeof(dataStruct->time));
    return len + sizeof(dataStruct->time);
}

uint16_t encodeError(const ChannelError* dataStruct, uint8_t* outBuffer, const uint16_t maxMsgLen) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;
    return snprintf((char*) outBuffer, maxMsgLen, 
                    "%c%s%s\n",
                    COMMS_MSG_SYNC, COMMS_MSG_ERROR_HEAD,
                    dataStruct->message);
}

uint8_t decodeInput(const uint8_t* dataBuffer, ChannelInput *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = COMMS_MSG_INPUT_HEAD[0];
    decodedMsg->channel = getChannelNumberFromBuffer(dataBuffer + 2);
    memcpy(&decodedMsg->time, dataBuffer + 5, sizeof(decodedMsg->time));
    return 1;
}

uint8_t decodeOutput(const uint8_t* dataBuffer, ChannelOutput *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = COMMS_MSG_OUTPUT_HEAD[0];
    decodedMsg->channel = getChannelNumberFromBuffer(dataBuffer + 2);
    decodedMsg->value = dataBuffer[4];
    memcpy(&decodedMsg->time, dataBuffer + 5, sizeof(decodedMsg->time));
    return 1;
}

uint8_t decodeFrequency(const uint8_t* dataBuffer, ChannelFrequency *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = COMMS_MSG_FREQ_HEAD[0];
    decodedMsg->channel = getChannelNumberFromBuffer(dataBuffer + 2);
    memcpy(&decodedMsg->time, dataBuffer + 12, sizeof(decodedMsg->time));
    return 1;
}

uint8_t decodeSettingsChannel(const uint8_t* dataBuffer, ChannelSettingsChannel *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command     = COMMS_MSG_CHANNEL_SETT_HEAD[0];
    decodedMsg->subCommand  = COMMS_MSG_CHANNEL_SETT_HEAD[1];
    decodedMsg->channel = getChannelNumberFromBuffer(dataBuffer + 3);
    
    const char* modeIdentifier = (char*) (dataBuffer + 5);
    if(strncmp(modeIdentifier, COMMS_SETT_CH_INPUT, strlen(COMMS_SETT_CH_INPUT)) == 0) {
        decodedMsg->mode = CHANNEL_INPUT;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_OUTPUT, strlen(COMMS_SETT_CH_OUTPUT)) == 0) {
        decodedMsg->mode = CHANNEL_OUTPUT;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_FREQUENCY, strlen(COMMS_SETT_CH_FREQUENCY)) == 0) {
    //     decodedMsg->mode = CHANNEL_FREQUENCY;
        // TODO: Frequency mode is still not implemented. Only input channels can calculate 
        // frequency at the moment.
        sendErrorMessage(COMMS_ERROR_INTERNAL);
        return 0;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_MONITOR_RISING, strlen(COMMS_SETT_CH_MONITOR_RISING)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_RISING_EDGES;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_MONITOR_FALLING, strlen(COMMS_SETT_CH_MONITOR_FALLING)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_FALLING_EDGES;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_MONITOR_BOTH, strlen(COMMS_SETT_CH_MONITOR_BOTH)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_BOTH_EDGES;
    }else if(strncmp(modeIdentifier, COMMS_SETT_CH_DISABLED, strlen(COMMS_SETT_CH_DISABLED)) == 0) {
        decodedMsg->mode = CHANNEL_DISABLED;
    }else {
        sendErrorMessage(COMMS_ERROR_CH_SETT_PARAMS);
        return 0;
    }

    if(decodedMsg->mode == CHANNEL_DISABLED) {
        decodedMsg->protocol = CHANNEL_PROTOC_OFF;
    }else if(dataBuffer[7] == (uint8_t) CHANNEL_PROTOC_5V) {
        decodedMsg->protocol = CHANNEL_PROTOC_5V;
    }else if(dataBuffer[7] == (uint8_t) CHANNEL_PROTOC_3V3) {
        decodedMsg->protocol = CHANNEL_PROTOC_3V3;
    }else if(dataBuffer[7] == (uint8_t) CHANNEL_PROTOC_1V8) {
        decodedMsg->protocol = CHANNEL_PROTOC_1V8;
    }else if(dataBuffer[7] == (uint8_t) CHANNEL_PROTOC_LVDS) {
        decodedMsg->protocol = CHANNEL_PROTOC_LVDS;
    }else {
        ChannelMessage errorMsg;
        strcpy((char*) errorMsg.error.message, COMMS_ERROR_CH_SETT_PARAMS);
        encodeGPIOMessage(GPIO_MSG_ERROR, errorMsg);
        return 0;
    }

    return 1;
}

uint8_t decodeSettingsSync(const uint8_t* dataBuffer, ChannelSettingsSYNC *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command     = COMMS_MSG_SYNC_SETT_HEAD[0];
    decodedMsg->subCommand  = COMMS_MSG_SYNC_SETT_HEAD[1];
    decodedMsg->channel = getChannelNumberFromBuffer(dataBuffer + 3);
    
    memcpy(&decodedMsg->frequency, dataBuffer + 5, sizeof(decodedMsg->frequency));
    if((decodedMsg->frequency < COMMS_SYNC_MIN_FREQ) || 
       (decodedMsg->frequency > COMMS_SYNC_MAX_FREQ)) {
        return 0;
    }
    
    memcpy(&decodedMsg->dutyCycle, dataBuffer + 13, sizeof(decodedMsg->dutyCycle));
    if((decodedMsg->dutyCycle < COMMS_SYNC_MIN_DUTY_CYCLE) || 
       (decodedMsg->dutyCycle > COMMS_SYNC_MAX_DUTY_CYCLE)) {
        return 0;
    }

    memcpy(&decodedMsg->time, dataBuffer + 21, sizeof(decodedMsg->time));
    return 1;
}

#if MCU_TX_IN_ASCII
inline uint16_t snprintf64Hex(char* outBuffer, uint16_t msgSize, uint64_t n) {
    atic char temp[16];
    uint8_t index = 0;
    while(n != 0) {
        temp[index] = n % 16;
        if(temp[index] <= 9) temp[index] += '0';
        else temp[index] += 'A' - 10;
        index++;
        n /= 16;
    }

    if(index > msgSize) {
        return 0;
    }

    for(uint8_t i = 0; i < index; i++) {
        outBuffer[i] = temp[index-1-i]; 
    }

    return index;
}
#endif

uint8_t executeInputCommand(const ChannelInput* cmdInput) {
    ChannelMessage cmdResponse;
    Channel* ch = getChannelFromNumber(cmdInput->channel);

    if(ch == NULL) {
        sendErrorMessage(COMMS_ERROR_INVALID_CHANNEL);
        return 0;
    }

    // Only if the channel is invalid, return error.
    if(ch->mode == CHANNEL_DISABLED) {
        sendErrorMessage(COMMS_ERROR_INVALID_MODE);
        return 0;
    }
    
    // All channels can be read.
    memcpy(&cmdResponse.input, cmdInput, sizeof(ChannelInput));

    uint8_t readState;
    if(getChannelState(ch, &readState)) {
        cmdResponse.input.value = readState ? GPIO_HIGH : GPIO_LOW;
    }else {
        sendErrorMessage(COMMS_ERROR_INTERNAL);
        return 0;
    }

    cmdResponse.input.time = convertFromInternalToUNIXTime(getMIDDSTime(&hwTimers));
    encodeGPIOMessage(GPIO_MSG_INPUT, cmdResponse);
    return 1;
}

uint8_t executeOutputCommand(const ChannelOutput* cmdInput) {
    Channel* ch = getChannelFromNumber(cmdInput->channel);

    // Is the channel number valid?
    if(ch == NULL) {
        sendErrorMessage(COMMS_ERROR_INVALID_CHANNEL);
        return 0;
    }

    // Only output channels can output values.
    if(ch->mode != CHANNEL_OUTPUT) {
        sendErrorMessage(COMMS_ERROR_INVALID_MODE);
        return 0;
    }

    // Parse the write the value.
    uint8_t writeState;
    if(cmdInput->value == GPIO_LOW) {
        writeState = 0;
    }else if(cmdInput->value == GPIO_HIGH) {
        writeState = 1;
    }else {
        sendErrorMessage(COMMS_ERROR_INVALID_VALUE);
        return 0;
    }

    // Write the value.
    if(!setChannelState(ch, writeState)) {
        sendErrorMessage(COMMS_ERROR_INTERNAL);
        return 0;
    }

    return 1;
}

uint8_t executeFrequencyCommand(const ChannelFrequency* cmdInput) {
    ChannelMessage cmdResponse;
    Channel* ch = getChannelFromNumber(cmdInput->channel);

    if(ch == NULL) {
        sendErrorMessage(COMMS_ERROR_INVALID_CHANNEL);
        return 0;
    }

    // Only if the channel is an output or disabled, return error.
    if((ch->mode != CHANNEL_INPUT) && (ch->mode != CHANNEL_FREQUENCY)) {
        sendErrorMessage(COMMS_ERROR_INVALID_MODE);
        return 0;
    }

    memcpy(&cmdResponse.frequency, cmdInput, sizeof(ChannelFrequency));
    if(ch->type == CHANNEL_TIMER) {
        getChannelFrequencyAndDutyCycle(ch->data.timer.timerHandler, 
                                        &cmdResponse.frequency.frequency, 
                                        &cmdResponse.frequency.dutyCycle);
    }else {
        // TODO: Make it so that GPIOs can also return frequency.
        sendErrorMessage(COMMS_ERROR_INVALID_MODE);
        return 0;
    }

    cmdResponse.frequency.time = convertFromInternalToUNIXTime(getMIDDSTime(&hwTimers));
    if(!encodeGPIOMessage(GPIO_MSG_FREQUENCY, cmdResponse)) {
    	return 0;
    }

    return 1;
}

uint8_t executeChannelSettingsCommand(const ChannelSettingsChannel* cmdInput) {
    Channel* ch = getChannelFromNumber(cmdInput->channel);

    if(ch == NULL) {
        sendErrorMessage(COMMS_ERROR_INVALID_CHANNEL);
        return 0;
    }

    // Timer channels can be set to either LVDS or TTL. GPIOs can only be TTL.
    if((ch->type == CHANNEL_GPIO) && (cmdInput->protocol == CHANNEL_PROTOC_LVDS)) {
        sendErrorMessage(COMMS_ERROR_INVALID_SIGNAL_TYPE);
        return 0;
    }

    ch->mode = cmdInput->mode;
    ch->protocol = cmdInput->protocol;

    applyChannelConfiguration(ch);
    setShiftRegisterValues(&chCtrl);
    return 1;
}

uint8_t executeSyncSettingsCommand(const ChannelSettingsSYNC* cmdInput) {
    if(cmdInput->channel != -1UL) {
        Channel* ch = getChannelFromNumber(cmdInput->channel);
        if(ch == NULL) {
            sendErrorMessage(COMMS_ERROR_INVALID_CHANNEL);
            return 0;
        }
    
        // Only "Timer" channels can be used as SYNC and when set to "monitoring both" mode.
        if((ch->type != CHANNEL_TIMER) || (ch->mode != CHANNEL_MONITOR_BOTH_EDGES)) {
            sendErrorMessage(COMMS_ERROR_SYNC_PARAMS);
            return 0;
        }
    }

    // Frequency and duty cycle must fall within the valid range.
    if((cmdInput->frequency < COMMS_SYNC_MIN_FREQ) || (cmdInput->frequency > COMMS_SYNC_MAX_FREQ) ||
       (cmdInput->dutyCycle <= COMMS_SYNC_MIN_DUTY_CYCLE) || 
       (cmdInput->dutyCycle >= COMMS_SYNC_MAX_DUTY_CYCLE)) {
        sendErrorMessage(COMMS_ERROR_SYNC_PARAMS);
        return 0;
    }

    // SYNC Time of -1 is reserved.
    if(cmdInput->time == -1ULL) {
        sendErrorMessage(COMMS_ERROR_SYNC_PARAMS);
        return 0;
    }

    setSyncParameters(&hwTimers, cmdInput->frequency, cmdInput->dutyCycle,
                      cmdInput->channel, convertFromUNIXTimeToInternal(cmdInput->time));
    return 1;
}

void sendErrorMessage(const char* errorMsg) {
    ChannelMessage cmdResponse;
    strcpy((char*) cmdResponse.error.message, errorMsg);
    encodeGPIOMessage(GPIO_MSG_ERROR, cmdResponse);
}

void establishConnection(uint8_t connect) {
    const char* WELCOME_MSG = "Connected to PROTO MIDDS v.0.1\n";

    usbConnected = connect;

    empty_cb(&outputBuffer);

    if(usbConnected) {
        // Transmit welcome message. 
        pushN_cb(&outputBuffer, (uint8_t*) WELCOME_MSG, strlen(WELCOME_MSG));
    }
    else
    {
        // Disconnected.
        NVIC_SystemReset();
    }

    // Set all channels as disabled.
    for(int i = 0; i < CH_COUNT; i++) {
        Channel* ch = getChannelFromNumber(i);
        if(ch == NULL) continue;

        ch->mode = CHANNEL_DISABLED;
        applyChannelConfiguration(ch);
    }
    setShiftRegisterValues(&chCtrl);
}

uint64_t convertFromInternalToUNIXTime(uint64_t tIn) {
    const double inToOutFactor = 1e9 / ((double) MCU_FREQUENCY);
    return tIn * inToOutFactor;
}

uint64_t convertFromUNIXTimeToInternal(uint64_t tEx) {
    const double outToInFactor = ((double) MCU_FREQUENCY) / 1e9;
    return tEx * outToInFactor;
}

uint32_t getChannelNumberFromBuffer(const uint8_t* buf) {
    if(buf[0] == '-') {
        return -(buf[1] - '0');
    }else {
        return (buf[0] - '0')*10 + (buf[1] - '0');
    }
}
