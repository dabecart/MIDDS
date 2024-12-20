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

uint16_t generateMonitorMessage(HWTimerChannel* hwTimer, char* outMsg, const uint16_t maxMsgLen) {
    if(hwTimer->data.len == 0) return 0;

    uint16_t msgSize = 0;
    uint32_t currentMessages;
#if MCU_TX_IN_ASCII
    uint64_t readVal;
#endif

    currentMessages = hwTimer->data.len;   // Make it constant at this point.
    if(currentMessages > 9999) currentMessages = 9999;
    msgSize += snprintf(outMsg + msgSize, maxMsgLen - msgSize, 
                        "$M%02d%04ld", hwTimer->channelNumber, currentMessages);
    // This upper string must be a multiple of eight bytes long (in binary output mode at least).

    for(uint8_t countIndex = 0; countIndex < currentMessages; countIndex++) {
        #if MCU_TX_IN_ASCII
            pop_cb64(&hwTimer->data, &readVal);
            msgSize += snprintf64Hex(
                            outMsg + msgSize, 
                            maxMsgLen - msgSize,
                            readVal
                        );
            if((maxMsgLen - msgSize) <= 16) {
                break;
            }
        #else
            pop_cb64(&hwTimer->data, (uint64_t*) (outMsg + msgSize));
            msgSize += 8;
            if((maxMsgLen - msgSize) <= 8) {
                break;
            }
        #endif
    }
    #if MCU_TX_IN_ASCII
        msgSize += snprintf(outMsg + msgSize, maxMsgLen-msgSize, "\n");
    #endif

    hwTimer->lastPrintTick = HAL_GetTick();
    return msgSize;
}

inline uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n) {
    static char temp[16];
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
        outMsg[i] = temp[index-1-i]; 
    }

    return index;
}