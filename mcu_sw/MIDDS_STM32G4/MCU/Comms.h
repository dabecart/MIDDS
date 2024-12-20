/***************************************************************************************************
 * @file Comms.h
 * @brief Implementation of the communication protocol of the MIDDS.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef COMMS_h
#define COMMS_h

#include "HWTimers.h"

/**************************************** FUNCTION *************************************************
 * @brief Prints the content of a HWTimerChannel to a string.
 * @param hwTimer. Pointer to the HWTimerChannel to print.
 * @param msg. Pointer to the string to print to.
 * @param maxMsgLen. Length of the msg buffer.
 * @return uint16_t. The number of characters written to msg.
***************************************************************************************************/
uint16_t generateMonitorMessage(HWTimerChannel* hwTimer, char* msg, const uint16_t maxMsgLen);

/**************************************** FUNCTION *************************************************
 * @brief Converts a uint64_t number into HEX. This number gets written into a string. The written
 * number will always start with x. If the HEX number exceeds msgSize, nothing gets written.
 * @param outMsg. Buffer to write the HEX number to.
 * @param msgSize. Length of outMsg buffer.
 * @param n. The number to convert into HEX.
 * @return uint16_t. The number of bytes written to outMsg.
***************************************************************************************************/
uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n);

#endif // COMMS_h