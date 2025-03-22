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
#include "CommsProtocol.h"
#include "CircularBuffer.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"

// Size of the USB messaging buffers. Currently set to the maximum allowed by the Circular Buffers.
#define USB_MAX_DATA_PACKAGE_SIZE CIRCULAR_BUFFER_MAX_SIZE

/**************************************** FUNCTION *************************************************
 * @brief Start the buffers for communication. 
 * @param blockUntilConnection: If 1, the program will hold until the COMMS_CONNECT_CMD message is
 * sent from the computer.
***************************************************************************************************/
void initComms(uint8_t blockUntilConnection);

/**************************************** FUNCTION *************************************************
 * @brief Main function to process the received data from USB. 
***************************************************************************************************/
void receiveData();

/**************************************** FUNCTION *************************************************
 * @brief Main function to send data through the USB. 
***************************************************************************************************/
void sendData();

/**************************************** FUNCTION *************************************************
 * @brief Encodes a message from a given data structure and message type. It saves it on the 
 * internal buffer to send to the output.
 * @param msgType: Type of message to encode.
 * @param msg: Where the message fields are stored.
 * @return 1 if the encoding was successful.
***************************************************************************************************/
uint8_t encodeGPIOMessage(const ChannelMessageType msgType, const ChannelMessage msg);

#define COMMS_DECODE_NOT_ENOUGH_DATA    -1
#define COMMS_DECODE_ERROR_DECODING     -2
#define COMMS_DECODE_SYNC_SEQUENCE_NOK  -3

/**************************************** FUNCTION *************************************************
 * @brief Decodes a message from a byte buffer to an struct which gets stored into a Circular
 * Buffer of this class.
 * @param dataBuffer: Where the data is stored.
 * @param ulDataLen: Length of the message.
 * @return If positive, the number of bytes read from the dataBuffer.
 *         If negative, it should be one of the following:
 *              - COMMS_DECODE_NOT_ENOUGH_DATA
 *              - COMMS_DECODE_ERROR_DECODING
 *              - COMMS_DECODE_ERROR_EXECUTING
 *              - COMMS_DECODE_SYNC_SEQUENCE_NOK
***************************************************************************************************/
int32_t decodeGPIOMessage(const uint8_t* dataBuffer, const uint32_t ulDataLen);

/**************************************** FUNCTION *************************************************
 * @brief Encodes a message to a byte buffer with a given INPUT data structure.
 * @param dataStruct: Where the message fields are stored.
 * @param outBuffer: Where the encoded message will be stored.
 * @return The byte length of the output buffer.
***************************************************************************************************/
uint16_t encodeInput(const ChannelInput* dataStruct, uint8_t* outBuffer);

/**************************************** FUNCTION *************************************************
 * @brief Prints the content of a HWTimerChannel to a string.
 * @param hwTimer. Pointer to the HWTimerChannel to print.
 * @param outBuffer: Where the encoded message will be stored.
 * @param maxMsgLen: Max length of the output buffer.
 * @return The number of characters written to msg.
***************************************************************************************************/
uint16_t encodeMonitor(HWTimerChannel* hwTimer, uint8_t* outBuffer, const uint16_t maxMsgLen);

/**************************************** FUNCTION *************************************************
 * @brief Encodes a message to a byte buffer with a given FREQUENCY data structure.
 * @param dataStruct: Where the message fields are stored.
 * @param outBuffer: Where the encoded message will be stored.
 * @return The byte length of the output buffer.
***************************************************************************************************/
uint16_t encodeFrequency(const ChannelFrequency* dataStruct, uint8_t* outBuffer);

/**************************************** FUNCTION *************************************************
 * @brief Encodes a message to a byte buffer with a given ERROR data structure.
 * @param dataStruct: Where the message fields are stored.
 * @param outBuffer: Where the encoded message will be stored.
 * @param maxMsgLen: Max length of the output buffer.
 * @return The byte length of the output buffer.
***************************************************************************************************/
uint16_t encodeError(const ChannelError* dataStruct, uint8_t* outBuffer, const uint16_t maxMsgLen);

/**************************************** FUNCTION *************************************************
 * @brief Decodes an INPUT message coming from a byte buffer.
 * @param outBuffer: Where the raw message is stored.
 * @param decodedMsg: Where the decoded message will be stored.
 * @return 1 if the message was well decoded.
***************************************************************************************************/
uint8_t decodeInput(const uint8_t* dataBuffer, ChannelInput *decodedMsg);

/**************************************** FUNCTION *************************************************
 * @brief Decodes an OUTPUT message coming from a byte buffer.
 * @param outBuffer: Where the raw message is stored.
 * @param decodedMsg: Where the decoded message will be stored.
 * @return 1 if the message was well decoded.
***************************************************************************************************/
uint8_t decodeOutput(const uint8_t* dataBuffer, ChannelOutput *decodedMsg);

/**************************************** FUNCTION *************************************************
 * @brief Decodes an FREQUENCY message coming from a byte buffer.
 * @param outBuffer: Where the raw message is stored.
 * @param decodedMsg: Where the decoded message will be stored.
 * @return 1 if the message was well decoded.
***************************************************************************************************/
uint8_t decodeFrequency(const uint8_t* dataBuffer, ChannelFrequency *decodedMsg);

/**************************************** FUNCTION *************************************************
 * @brief Decodes an SETTINGS CHANNEL message coming from a byte buffer.
 * @param outBuffer: Where the raw message is stored.
 * @param decodedMsg: Where the decoded message will be stored.
 * @return 1 if the message was well decoded.
***************************************************************************************************/
uint8_t decodeSettingsChannel(const uint8_t* dataBuffer, ChannelSettingsChannel *decodedMsg);

/**************************************** FUNCTION *************************************************
 * @brief Decodes an SETTINGS SYNC message coming from a byte buffer.
 * @param outBuffer: Where the raw message is stored.
 * @param decodedMsg: Where the decoded message will be stored.
 * @return 1 if the message was well decoded.
***************************************************************************************************/
uint8_t decodeSettingsSync(const uint8_t* dataBuffer, ChannelSettingsSYNC *decodedMsg);

#if MCU_TX_IN_ASCII
/**************************************** FUNCTION *************************************************
 * @brief Converts a uint64_t number into HEX. This number gets written into a string. The written
* number will always start with x. If the HEX number exceeds msgSize, nothing gets written.
 * @param outMsg. Buffer to write the HEX number to.
 * @param msgSize. Length of outMsg buffer.
 * @param n. The number to convert into HEX.
 * @return uint16_t. The number of bytes written to outMsg.
***************************************************************************************************/
uint16_t snprintf64Hex(char* outMsg, uint16_t msgSize, uint64_t n);
#endif

/**************************************** FUNCTION *************************************************
 * @brief Executes an INPUT message: generates the response.
 * @param cmdInput: The message/command to execute.
 * @return 1 if the message was well executed.
***************************************************************************************************/
uint8_t executeInputCommand(const ChannelInput* cmdInput);

/**************************************** FUNCTION *************************************************
 * @brief Executes an OUTPUT message: sets the GPIO value.
 * @param cmdInput: The message/command to execute.
 * @return 1 if the message was well executed.
***************************************************************************************************/
uint8_t executeOutputCommand(const ChannelOutput* cmdInput);

/**************************************** FUNCTION *************************************************
 * @brief Executes a FREQUENCY message: generates the response.
 * @param cmdInput: The message/command to execute.
 * @return 1 if the message was well executed.
***************************************************************************************************/
uint8_t executeFrequencyCommand(const ChannelFrequency* cmdInput);

/**************************************** FUNCTION *************************************************
 * @brief Executes a CHANNEL SETTINGS command.
 * @param cmdInput: The message/command to execute.
 * @return 1 if the message was well executed.
***************************************************************************************************/
uint8_t executeChannelSettingsCommand(const ChannelSettingsChannel* cmdInput);

/**************************************** FUNCTION *************************************************
 * @brief Executes a SYNC SETTINGS command.
 * @param cmdInput: The message/command to execute.
 * @return 1 if the message was well executed.
***************************************************************************************************/
uint8_t executeSyncSettingsCommand(const ChannelSettingsSYNC* cmdInput);

/**************************************** FUNCTION *************************************************
 * @brief Generates and sends an error message.
 * @param errorMsg: The error message.
***************************************************************************************************/
void sendErrorMessage(const char* errorMsg);

/**************************************** EXTERNALS ***********************************************/
extern CircularBuffer inputBuffer;

#endif // COMMS_h