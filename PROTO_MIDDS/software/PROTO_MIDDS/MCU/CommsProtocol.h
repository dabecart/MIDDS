/***************************************************************************************************
 * @file CommsProtocol.h
 * @brief Defines the protocol used for communication.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef COMMS_PROTOCOL_h
#define COMMS_PROTOCOL_h

#include "stdint.h"

#define GPIO_MSG_SYNC   '$'

#define GPIO_MSG_INPUT_LEN          13
#define GPIO_MSG_OUTPUT_LEN         13
#define GPIO_MSG_FREQ_LEN           12
#define GPIO_MSG_CHANNEL_SETT_LEN   9
#define GPIO_MSG_SYNC_SETT_LEN      21

#define GPIO_MSG_INPUT_HEAD         "I"
#define GPIO_MSG_OUTPUT_HEAD        "O"
#define GPIO_MSG_FREQ_HEAD          "F"
#define GPIO_MSG_MONITOR_HEAD       "M"
#define GPIO_MSG_CHANNEL_SETT_HEAD  "SC"
#define GPIO_MSG_SYNC_SETT_HEAD     "SY"
#define GPIO_MSG_ERROR_HEAD         "E"

#define GPIO_SETT_CH_INPUT              "IN"
#define GPIO_SETT_CH_OUTPUT             "OU"
#define GPIO_SETT_CH_FREQUENCY          "FR"
#define GPIO_SETT_CH_MONITOR_RISING     "MR"
#define GPIO_SETT_CH_MONITOR_FALLING    "MF"
#define GPIO_SETT_CH_MONITOR_BOTH       "MB"
#define GPIO_SETT_CH_DISABLED           "DS"

// Discrete outputs of each channel.
typedef enum ChannelValue 
{
    GPIO_LOW       = '0',
    GPIO_HIGH      = '1',
    GPIO_EMPTY     = ' '
} ChannelValue;

// Edge that can be detected by "monitoring" channels.
typedef enum ChannelEdge 
{
    CHANNEL_EDGE_FALLING    = '0',
    CHANNEL_EDGE_RISING     = '1'
} ChannelEdge;

// Type of protocol used to generate the signal on each GPIO channel.
typedef enum GPIOSignalType
{
    CHANNEL_SIGNAL_TTL    = 'T',
    CHANNEL_SIGNAL_LVDS   = 'L'
} GPIOSignalType;

// Use the channel as a SYNC signal? Used to generate the timestamps.
typedef enum GPIOSYNC
{
    CHANNEL_SYNC_USED     = 'Y',
    CHANNEL_SYNC_NOT_USED = 'N'
} GPIOSYNC;

// Modes in which a channel can operate.
typedef enum ChannelMode
{
    CHANNEL_INPUT,
    CHANNEL_OUTPUT,
    CHANNEL_FREQUENCY,
    CHANNEL_MONITOR_RISING_EDGES,
    CHANNEL_MONITOR_FALLING_EDGES,
    CHANNEL_MONITOR_BOTH_EDGES,
    CHANNEL_DISABLED
} ChannelMode;

// Struct of Input messages.
typedef struct ChannelInput {
    uint8_t         command;
    uint32_t        channel;
    ChannelValue    value;
    double          time;
} ChannelInput;

// Struct of Output messages.
typedef struct ChannelOutput {
    uint8_t         command;
    uint32_t        channel;
    ChannelValue    value;
    double          time;
} ChannelOutput;

// Struct of Monitor messages (Not used).
// typedef struct GPIOMonitor{
//     uint8_t     command;
//     uint32_t    channel;
//     uint32_t    sampleCount;
//     double*     samples;
// } GPIOMonitor;

// Struct of Frequency messages.
typedef struct ChannelFrequency {
    uint8_t     command;
    uint32_t    channel;
    double      frequency;
} ChannelFrequency;

// Struct of Settings: Channel messages.
typedef struct ChannelSettingsChannel{
    uint8_t         command;
    uint8_t         subCommand;
    uint32_t        channel;
    ChannelMode     mode;
    GPIOSignalType  signal;
    GPIOSYNC        sync;
} ChannelSettingsChannel;

// Struct of Settings: SYNC messages.
typedef struct ChannelSettingsSYNC{
    uint8_t command;
    uint8_t subCommand;
    double  frequency;
    double  dutyCycle;
    double  time;
} ChannelSettingsSYNC;

// Struct of error messages.
typedef struct ChannelError{
    uint8_t command;
    uint8_t message[256];
} ChannelError;

// Enum with all the messages' types.
typedef enum ChannelMessageType
{
    GPIO_MSG_INPUT,
    GPIO_MSG_OUTPUT,
    GPIO_MSG_FREQUENCY,
    GPIO_MSG_MONITOR,
    GPIO_MSG_CHANNEL_SETTINGS,
    GPIO_MSG_SYNC_SETTINGS,
    GPIO_MSG_ERROR
} ChannelMessageType;

// Union with the different types of messages.
typedef union ChannelMessage
{
    ChannelInput            input;
    ChannelOutput           output;
    ChannelFrequency        frequency;
    HWTimerChannel*         monitor;
    ChannelSettingsChannel  channelSettings;
    ChannelSettingsSYNC     syncSettings;
    ChannelError            error;
} ChannelMessage;

#endif // COMMS_PROTOCOL_h
