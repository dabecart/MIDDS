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

#define COMMS_MSG_SYNC              '$'

#define COMMS_MSG_INPUT_LEN          13
#define COMMS_MSG_OUTPUT_LEN         13
#define COMMS_MSG_FREQ_LEN           28
#define COMMS_MSG_CHANNEL_SETT_LEN   8
#define COMMS_MSG_SYNC_SETT_LEN      29

#define COMMS_MIN_MSG_LEN            5 // $CONN or $DISC
#define COMMS_MAX_MSG_INPUT_LEN      COMMS_MSG_SYNC_SETT_LEN

#define COMMS_MSG_INPUT_HEAD         "I"
#define COMMS_MSG_OUTPUT_HEAD        "O"
#define COMMS_MSG_FREQ_HEAD          "F"
#define COMMS_MSG_MONITOR_HEAD       "M"
#define COMMS_MSG_CHANNEL_SETT_HEAD  "SC"
#define COMMS_MSG_SYNC_SETT_HEAD     "SY"
#define COMMS_MSG_ERROR_HEAD         "E"
#define COMMS_MSG_CONNECT_HEAD       "CONN"
#define COMMS_MSG_DISCONNECT_HEAD    "DISC"

// Input channels can return their state and also their frequency and duty cycle. The frequency gets
// calculated with timestamps. At the moment, only Timer channels can return the frequency.
#define COMMS_SETT_CH_INPUT              "IN"
// Output channels can set their state. Not implemented at the moment.
#define COMMS_SETT_CH_OUTPUT             "OU"
// Frequency channels are similar to input channels but they calculate their frequency not by their
// timestamps but by an internal counter, meaning higher frequencies can be detected. Still not 
// implemented.
#define COMMS_SETT_CH_FREQUENCY          "FR"
// Monitoring channels automatically send their "monitoring messages" containing the timestamp of 
// their edges and its current state.
#define COMMS_SETT_CH_MONITOR_RISING     "MR"
#define COMMS_SETT_CH_MONITOR_FALLING    "MF"
#define COMMS_SETT_CH_MONITOR_BOTH       "MB"
// A disabled channel is kept in High-Z.
#define COMMS_SETT_CH_DISABLED           "DS"

#define COMMS_SYNC_MIN_FREQ         00.01
#define COMMS_SYNC_MAX_FREQ         99.99
#define COMMS_SYNC_MIN_DUTY_CYCLE   0
#define COMMS_SYNC_MAX_DUTY_CYCLE   100.0

#define COMMS_ERROR_INVALID_CHANNEL      "RR_INVALID_CHANNEL"
#define COMMS_ERROR_INVALID_MODE         "RR_INVALID_MODE"
#define COMMS_ERROR_INVALID_SIGNAL_TYPE  "RR_INVALID_SIGNAL_TYPE"
#define COMMS_ERROR_CH_SETT_PARAMS       "RR_CH_SETT_PARAMS"
#define COMMS_ERROR_SYNC_PARAMS          "RR_SYNC_PARAMS"
#define COMMS_ERROR_INTERNAL             "RR_INTERNAL"

#define COMMS_ERROR_MAX_LEN         64

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

// Modes in which a channel can operate.
typedef enum ChannelMode
{
    CHANNEL_INPUT = 1,
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
    uint64_t        time;
} ChannelInput;

// Struct of Output messages
typedef struct ChannelOutput {
    uint8_t         command;
    uint32_t        channel;
    ChannelValue    value;
    uint64_t        time;
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
    double      dutyCycle;
    uint64_t    time;
} ChannelFrequency;

// Struct of Settings: Channel messages.
typedef struct ChannelSettingsChannel{
    uint8_t         command;
    uint8_t         subCommand;
    uint32_t        channel;
    ChannelMode     mode;
    GPIOSignalType  signal;
} ChannelSettingsChannel;

// Struct of Settings: SYNC messages.
typedef struct ChannelSettingsSYNC{
    uint8_t     command;
    uint8_t     subCommand;
    uint32_t    channel;
    double      frequency;
    double      dutyCycle;
    uint64_t    time;
} ChannelSettingsSYNC;

// Struct of error messages.
typedef struct ChannelError{
    uint8_t command;
    uint8_t message[COMMS_ERROR_MAX_LEN];
} ChannelError;

// Enum with all the messages' types.
typedef enum ChannelMessageType
{
    GPIO_MSG_INPUT,
    GPIO_MSG_OUTPUT,
    GPIO_MSG_FREQUENCY,
    GPIO_MSG_MONITOR,
    GPIO_MSG_CHANNEL_SETTINGS,
    COMMS_MSG_SYNC_SETTINGS,
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
