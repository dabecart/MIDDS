/***************************************************************************************************
 * @file Comms.c
 * @brief Implementation of the communication protocol of the MIDDS.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "Comms.h"
#include "MainMCU.h"

CircularBuffer outputBuffer;
extern USBD_HandleTypeDef hUsbDeviceFS;

void initComms(uint8_t blockUntilConnection) {
    init_cb(&outputBuffer, CIRCULAR_BUFFER_MAX_SIZE);

    // Wait until the USB gets connected.
    while(blockUntilConnection && (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)){}

    char* WELCOME_MSG = "PROTO_MIDDS v.0.1\n";
    while(CDC_Transmit_FS((uint8_t*) WELCOME_MSG, strlen(WELCOME_MSG)) == USBD_BUSY){}
}

void receiveData() {
    
}

void sendData() {
    static uint8_t outMsg[USB_MAX_DATA_PACKAGE_SIZE];
    static uint16_t outMsgLen = 0;

    if(outMsgLen == 0) {
        outMsgLen = outputBuffer.len;
        if(outMsgLen > USB_MAX_DATA_PACKAGE_SIZE) outMsgLen = USB_MAX_DATA_PACKAGE_SIZE;
        popN_cb(&outputBuffer, outMsgLen, outMsg);
    }

    if(CDC_Transmit_FS(outMsg, outMsgLen) != USBD_BUSY) {
        outMsgLen = 0;
    }
}

uint8_t encodeGPIOMessage(const ChannelMessageType msgType, const ChannelMessage msg) {
    static uint8_t outMsgBuffer[CIRCULAR_BUFFER_MAX_SIZE]; 
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
            messageLen = encodeMonitor(&msg.monitor, outMsgBuffer, CIRCULAR_BUFFER_MAX_SIZE);
            break;
        }

        case GPIO_MSG_ERROR: {
            messageLen = encodeError(&msg.error, outMsgBuffer, CIRCULAR_BUFFER_MAX_SIZE);
            break;
        }

        default: {
            // TODO: Invalid message type.
            return 0;
        }
    }

    if((messageLen == 0) || (messageLen > CIRCULAR_BUFFER_MAX_SIZE)) {
        return 0;
    }

    // Push to the output buffer.
    pushN_cb(&outputBuffer, outMsgBuffer, messageLen);
    return 1;
}

uint8_t decodeGPIOMessage(const uint8_t* dataBuffer, const uint32_t dataLen) {
    if((dataBuffer == NULL) || (dataLen == 0) || (dataBuffer[0] != GPIO_MSG_SYNC)) {
        return 0;
    }

    const uint8_t messageID = (uint8_t*) dataBuffer+1;
    if(strncmp(messageID, GPIO_MSG_INPUT_HEAD, strlen(GPIO_MSG_INPUT_HEAD)) == 0) {
        ChannelInput temp = {};
        if(decodeInput(dataBuffer, &temp)) {
            
        }
    }    
    else if(strncmp(messageID, GPIO_MSG_OUTPUT_HEAD, strlen(GPIO_MSG_OUTPUT_HEAD)) == 0) {
        ChannelInput temp = {};
        if(decodeOutput(dataBuffer, &temp)) {

        }
    }
    else if(strncmp(messageID, GPIO_MSG_FREQ_HEAD, strlen(GPIO_MSG_FREQ_HEAD)) == 0) {
        ChannelFrequency temp = {};
        if(decodeFrequency(dataBuffer, &temp)) {

        }
    }
    else if(strncmp(messageID, GPIO_MSG_CHANNEL_SETT_HEAD, strlen(GPIO_MSG_CHANNEL_SETT_HEAD)) == 0) {
        ChannelSettingsChannel temp = {};
        if(decodeSettingsChannel(dataBuffer, &temp)) {
            
        }
    }
    else if(strncmp(messageID, GPIO_MSG_SYNC_SETT_HEAD, strlen(GPIO_MSG_SYNC_SETT_HEAD)) == 0) {
        ChannelSettingsSYNC temp = {};
        if(decodeSettingsSync(dataBuffer, &temp)) {
            
        }
    }
    else {
        // TODO: Invalid type
        return 0;
    }
    return 1;
}

uint16_t encodeInput(const ChannelInput* dataStruct, uint8_t* outBuffer) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;

    uint16_t len = sprintf((char*) outBuffer, 
                    "%c%s%02d%c",
                    GPIO_MSG_SYNC, GPIO_MSG_INPUT_HEAD,
                    dataStruct->channel, dataStruct->value);
    memcpy(outBuffer + len, (uint8_t*) &dataStruct->time, sizeof(dataStruct->time));
    return len + sizeof(dataStruct->time);
}

uint16_t encodeMonitor(HWTimerChannel* hwTimer, uint8_t* outBuffer, const uint16_t maxMsgLen){
    if(hwTimer == NULL || hwTimer->data.len == 0) return 0;

    uint32_t currentMessages = hwTimer->data.len;   // Make it constant at this point.
    if(currentMessages > 9999) currentMessages = 9999;
    
    // To make the pop_cb64, this header must be a multiple of eight bytes long (in binary output 
    // mode at least).
    uint16_t msgSize = snprintf((char*) outBuffer, maxMsgLen, 
                                "%c%s%02d%04ld", 
                                GPIO_MSG_SYNC, GPIO_MSG_MONITOR_HEAD,
                                hwTimer->channelNumber, currentMessages);

#if MCU_TX_IN_ASCII
    uint64_t readVal;
    for(uint8_t countIndex = 0; countIndex < currentMessages; countIndex++) {
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
    for(uint8_t countIndex = 0; countIndex < currentMessages; countIndex++) {
        pop_cb64(&hwTimer->data, (uint64_t*) (outBuffer + msgSize));
        msgSize += sizeof(uint64_t);
        if((maxMsgLen - msgSize) <= sizeof(uint64_t)) {
            break;
        }
    }
#endif

    hwTimer->lastPrintTick = HAL_GetTick();
    return msgSize;
}

uint16_t encodeFrequency(const ChannelFrequency* dataStruct, uint8_t* outBuffer) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;
    uint16_t len = snprintf((char*) outBuffer, 
                            "%c%s%02d", 
                            GPIO_MSG_SYNC, GPIO_MSG_ERROR_HEAD,
                            dataStruct->channel);
    memcpy(outBuffer + len, (uint8_t*) &dataStruct->frequency, sizeof(dataStruct->frequency));
    return len + sizeof(dataStruct->frequency);
}

uint16_t encodeError(const ChannelError* dataStruct, uint8_t* outBuffer, const uint16_t maxMsgLen) {
    if(dataStruct == NULL || outBuffer == NULL) return 0;
    return snprintf((char*) outBuffer, maxMsgLen, 
                    "%c%s%s", 
                    GPIO_MSG_SYNC, GPIO_MSG_ERROR_HEAD,
                    dataStruct->message);
}

uint8_t decodeInput(const uint8_t* dataBuffer, ChannelInput *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = GPIO_MSG_INPUT_HEAD[0];
    sscanf((char*) dataBuffer + 2, "%2d", &decodedMsg->channel);
    memcpy(&decodedMsg->time, dataBuffer + 5, sizeof(decodedMsg->time));
    return 1;
}

uint8_t decodeOutput(const uint8_t* dataBuffer, ChannelOutput *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = GPIO_MSG_OUTPUT_HEAD[0];
    sscanf((char*) dataBuffer + 2, "%2d%c", &decodedMsg->channel, &decodedMsg->value);
    memcpy(&decodedMsg->time, dataBuffer + 5, sizeof(decodedMsg->time));
    return 1;
}

uint8_t decodeFrequency(const uint8_t* dataBuffer, ChannelFrequency *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command = GPIO_MSG_FREQ_HEAD[0];
    sscanf((char*) dataBuffer + 2, "%2d", &decodedMsg->channel);
    return 1;
}

uint8_t decodeSettingsChannel(const uint8_t* dataBuffer, ChannelSettingsChannel *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    decodedMsg->command     = GPIO_MSG_CHANNEL_SETT_HEAD[0];
    decodedMsg->subCommand  = GPIO_MSG_CHANNEL_SETT_HEAD[1];
    sscanf((char*) dataBuffer + 3, "%2d", &decodedMsg->channel);
    
    const char* modeIdentifier = dataBuffer + 5;
    if(strncmp(modeIdentifier, GPIO_SETT_CH_INPUT, strlen(GPIO_SETT_CH_INPUT)) == 0) {
        decodedMsg->mode = CHANNEL_INPUT;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_OUTPUT, strlen(GPIO_SETT_CH_OUTPUT)) == 0) {
        decodedMsg->mode = CHANNEL_OUTPUT;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_FREQUENCY, strlen(GPIO_SETT_CH_FREQUENCY)) == 0) {
        decodedMsg->mode = CHANNEL_FREQUENCY;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_MONITOR_RISING, strlen(GPIO_SETT_CH_MONITOR_RISING)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_RISING_EDGES;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_MONITOR_FALLING, strlen(GPIO_SETT_CH_MONITOR_FALLING)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_FALLING_EDGES;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_MONITOR_BOTH, strlen(GPIO_SETT_CH_MONITOR_BOTH)) == 0) {
        decodedMsg->mode = CHANNEL_MONITOR_BOTH_EDGES;
    }else if(strncmp(modeIdentifier, GPIO_SETT_CH_DISABLED, strlen(GPIO_SETT_CH_DISABLED)) == 0) {
        decodedMsg->mode = CHANNEL_DISABLED;
    }else {
        // TODO: Wrong mode.
        return 0;
    }

    if(dataBuffer[7] == (uint8_t) CHANNEL_SIGNAL_TTL) {
        decodedMsg->mode = CHANNEL_SIGNAL_TTL;
    }else if(dataBuffer[7] == (uint8_t) CHANNEL_SIGNAL_LVDS) {
        decodedMsg->mode = CHANNEL_SIGNAL_LVDS;
    }else {
        // TODO: Wrong signal identifier.
        return 0;
    }

    if(dataBuffer[8] == (uint8_t) CHANNEL_SYNC_USED) {
        decodedMsg->sync = CHANNEL_SYNC_USED;
    }else if(dataBuffer[8] == (uint8_t) CHANNEL_SYNC_NOT_USED) {
        decodedMsg->sync = CHANNEL_SYNC_NOT_USED;
    }else {
        // TODO: Wrong SYNC.
        return 0;
    }    

    return 1;
}

uint8_t decodeSettingsSync(const uint8_t* dataBuffer, ChannelSettingsSYNC *decodedMsg) {
    if((dataBuffer == NULL) || (decodedMsg == NULL)) return 0;

    // TODO
    return 0;
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
