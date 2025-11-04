# MIDDS
*Monitoring Interface of Digital and Differential Signals, by @dabecart.*

## Overview

The **MIDDS** is a peripheral board designed to connect to a computer via USB, enhancing your software with timestamped GPIO capabilities. More than just a reliable General-Purpose Input/Output device, it delivers precise and high-accuracy timestamping. Featuring 14 configurable channels, the MIDDS supports +5V, +3V3, +1V8 single-ended or LVDS input and output modes. These channels enable timestamping for both inputs and outputs, with each channel capable of being selected as the SYNC input. This SYNC input can accept a rectangular signal from an external clock source to synchronize the MIDDS's time with the external clock. MIDDS also has 16 additional GPIOs (only +5V, +3V3 and +1V8, not LVDS).

For even greater precision, the MIDDS supports an external clock signal as HCLK, which can be connected through an onboard SMA connector. The board also includes a TFT LCD screen for convenient display of the monitor's current state. All external connections are ESD-protected.

The MIDDS solves two problems on the same device:

- For one, it works as a digital oscilloscope of high timing accuracy relative to its price.
- And it works as a GPIO card, allowing the computer to generate both input and output signals, TTL and LVDS!

It can therefore be used in the following applications:

- Cost-effective laboratory equipment and instrumentation.
- Sensor calibration.
- Multi-protocol interface adapter (SPI, I2C).
- Board to board communication.
- GNSS.
- Test and measurement.

## Capabilities

- **General Purpose Timers (TIMx):** they give accurate measures up to the frequency of the MCU. Currently set at 160 MHz, MIDDS gives a time precision of 6.25 nanoseconds.  
- **IO:** in MIDDS there are two types of input/outputs:
  - **Timing channels [Ch00 to Ch15]**. They offer high timing precision and can be configured as +5V, +3V3, +1V8 or LVDS inputs/outputs.
  - **General IO [Ch16 to Ch31]**. They are general purpose input and output pins.
- **USB**: MIDDS can be powered through an USB-C cable. It uses CDC (Communications Devices Class) to communicate with the computer and it will appear as a virtual serial port device. Tested with up to 10 MBauds.
- **SWD**: makes MIDDS easily debuggable and open to tinker with!
- **External power source:** MIDDS counts with two independent +3.3V power sources to connect other devices to the board.
- **External clock:** use the SMA connector (Ext 10MHz) to input an external clock signal of your convenience. Select the clock source by shortcircuiting the SCK pins, in either the Int (Internal clock) or Ext (External clock) position. 
- **LED indicators:** to easily see the configuration and state of all individual channels.

With all these features, below are listed the expected measuring characteristics of the MIDDS:

- **Guaranteed less than 6.25 ns of deviation** between real and measured timestamps in timing inputs.
- Clock deviation after SYNC signal: 
  - Using onboard TCXO (X1): worst-case scenario of 10 us/s, 600 ns/min, 36 us/h, 864 us/day. 
  - It is therefore recommended to use a SYNC signal of a least 1 Hz for high precision measurements.
- Guaranteed **exact readings of simultaneous signals**.
- Recording of **both rising and falling edges**.
- Generation of TTL and LVDS **outputs** on request.
- **ESD protection** on all inputs.
- **High impedance input**, meaning that MIDDS will not affect the circuit is connected to.

## Working principle

MIDDS heavily relies on the hardware timers of the MCU. A timer is basically a 16-bit counter, a module within the chip that increments its value by one at the same rate of the MCU’s clock, in case of the HRTIM; and at half its rate, in the case of TIMx.

The counter’s numerical range is somewhat limited (from 0 to 65535). Although this at first may not appear to be a small number, the MCU’s clock is expected to run at around 400MHz, meaning that the counter will fill up every 163.84 microseconds!

When the counter reaches its upper threshold, an overflow occurs, and the counter rolls back to zero. Every time this happens, an interruption or ISR is triggered on the MCU. On this ISR, the MCU increments a software 64-bit counter called the "coarse counter" by exactly 65536. By adding the TIMx value to the coarse counter, the real time of the timestamp gets calculated. There's one big "if" to this approach and that is that MIDDS is expected to serve the ISR "relatively fast". 

Therefore, MIDDS has a precision-per-bit of 2.5ns (at 400MHz) and a total time count of 4.612 · 10<sup>10</sup> seconds (around 1461 years).

Although this number is impressive, it is not very useful to use such large times without keeping a more constraint time of reference. This is because of the relative accuracy of the clock pulse which is fed to the MCU: 
- Using the onboard TXCO (X1) of 8MHz, 10 ppm, for each pulse of a real 8MHz clock, there is a small amount of time which, at maximum, is being lost: every 8 million pulses, there can be a maximum miss or gain of 80 pulses. Therefore, there is a maximum drift of 10 us/s, 600 us/min, 36 ms/h or 864 ms/day which for some applications can be too much.
- An external clock signal may be used by inputting it through X2. For example, by using an external OCXO of 25MHz, 10 ppb, every second there will be a maximum of 0.25 pulses "lost". That equals a maximum delay is of ±10 ns/s, 600 ns/min, 36 us/h or 864 us/day.

Due to these drifts in the clock sources, MIDDS also has a **SYNC** input that allows the user to input its reference pulse signal. This signal will normally comes from a more stable and reliable source, such as GPS. The frequency, duty cycle and the SYNC channel will need to be specified via software.

By syncing to an external SYNC source, the drift is kept at a minimum as it resets everytime there is a new SYNC edge as the SYNC algorithm works on both edges of the SYNC signal. As an example, by using a 1PPS signal from a GNSS sensor with 50% duty cycle, MIDDS gets synchronized once every 500ms, giving a maximum delay of 5us with the onboard TCXO. Therefore, there are two simple ways to improve the accuracy of MIDDS:
- Increment the SYNC frequency.
- Use a better clock source for MIDDS.

# Communication protocol
The MIDDS protocol has been designed to be as quick and lightweight as possible to be generated and parsed, while also being easy to be read by a human.

- Bytes are formatted in little-endian.
- Bytes are sent from left to right. In the case of the tables below, from up to bottom.
- There is no acknowledgement between messages:
  - If the message is not well formatted, the receiving end of the communication should discard it. MIDDS will generate an error message on these cases.
  - A message must be fully correct for the MIDDS to apply it. If even a minor field is not formatted accordingly, MIDDS will not apply any changes to its internal configuration.

A `time` field is a 64-bit unsigned integer which stores [UMIX time](https://en.wikipedia.org/wiki/Unix_time) in **nanoseconds**. Depending on the sender of the message, this field operates as:
  - *Execution time*, when the message is sent from the PC to MIDDS. When the time of MIDDS reaches this execution time, the command will be executed. This time is absolute! If an execution time which has already passed is given to a command, the command will be executed immediately. It is recommended to use an execution time of zero for the command to be executed immediately. 
  - *Response Timestamp*, when the message is sent from MIDDS to the PC. Timestamps the time when the content of the message was calculated/generated.

## Commands/Messages

### Input (`I`)
Gives the value of a MIDDS *input*, *output* or *monitoring* channel. This read can be instant or delayed until a certain time.
- It is asked first by the computer and answered by MIDDS.
- Command format. 13 bytes long.

| Field              | Value                                | Type   | Byte size | Byte Offset |
|--------------------|--------------------------------------|--------|-----------|-------------|
| Start character    | `$`                                  | `char` | 1         | 0           |
| Command descriptor | `I`                                  | `char` | 1         | 1           |
| Channel number     | `00` to `99`                         | `char` | 2         | 2           |
| Read value         | PC: do not care<br>MIDDS: `0` or `1` | `char` | 1         | 4           |
| Time               | ---                                  | `time` | 8         | 5           |

### Output (`O`)

Sets the value of an *output* channel. This output can be instant or delayed until a certain time.
- Sent by the computer.
- If this command is sent to a *monitoring* or *input* channel it will be discarded.
- If the command is sent to a *disabled* channel, it sets its inner output value so that if it is set as an output, that will be its initial value. If not sent, the initial value of the output channel cannot be asserted, unless it is read beforehand with an `I` command.
- Command format. 13 bytes long.
  
| Field              | Value        | Type   | Byte size | Byte Offset |
|--------------------|--------------|--------|-----------|-------------|
| Start character    | `$`          | `char` | 1         | 0           |
| Command descriptor | `O`          | `char` | 1         | 1           |
| Channel number     | `00` to `99` | `char` | 2         | 2           |
| Write value        | `0` or `1`   | `char` | 1         | 4           |
| Time               | ---          | `time` | 8         | 5           |

### Frequency (`F`)

Gives the frequency of a MIDDS *input* channel. This read can be instant or delayed until a certain time.
- Sent by the computer and answered by MIDDS.
- If this command is sent to anything other than an *input* channel, it will be discarded.
- Command format. 28 bytes long.
  
| Field              | Value                                | Type     | Byte size | Byte Offset |
|--------------------|--------------------------------------|--------  |-----------|-------------|
| Start character    | `$`                                  | `char`   | 1         | 0           |
| Command descriptor | `F`                                  | `char`   | 1         | 1           |
| Channel number     | `00` to `99`                         | `char`   | 2         | 2           |
| Frequency          | PC: do not care<br>MIDDS: `Hz`       | `double` | 8         | 4           |
| Duty cycle         | PC: do not care<br>MIDDS: `%`        | `double` | 8         | 12          |
| Time               | ---                                  | `time`   | 8         | 20          |

### Monitor (`M`)

Used to send multiple timestamps of *monitoring* inputs/outputs. This message can bundle up to 9999
timestamps.
- Sent only by MIDDS.
- Command format. 16 bytes long minimum.

| Field              | Value                                   | Type     | Byte size | Byte Offset |
|--------------------|-----------------------------------------|----------|-----------|-------------|
| Start character    | `$`                                     | `char`   | 1         | 0           |
| Command descriptor | `M`                                     | `char`   | 1         | 1           |
| Channel number     | `00` to `99`                            | `char`   | 2         | 2           |
| Number of samples  | `0001` to `9999`                        | `char`   | 4         | 4           |
| `#n` sample        | *See below*                             | `sample` | 8         | 8 + `#n`*8  |

  A `sample` is a `time` variable that has been bit-shifted one place to the left and ORed with the type of edge that triggered the sample:
  - Falling edge: `0`
  - Rising edge: `1`
  
### Settings (`S`)

The settings command is used to change the configuration of the MIDDS. All settings commands must start with `$S` plus another letter, which specifies the type of setting that is being commanded.

#### Channel Settings (`SC`)

Sets the configuration of a channel of the MIDDS.
- Can only be initially sent by the computer.
- This command can act as:
  - Write configuration. Overrides the previous configuration of the channel.
  - Read configuration. Asks the MIDDS for the current configuration of the channel. The MIDDS returns it.
- This command **cannot be delayed**, its effect is immediate.
- Fields of the Configuration Channel:
  - Set the **mode** of a MIDDS channel:
    - **Input**. Reads the value of a given channel in the specified time mark or as quick as possible.
    - **Output**. The channel outputs a given value in the specified time mark or as quick as possible.
    - **Monitoring**. It is an special kind of input. When a change in the voltage of the channel occurs, MIDDS sends a message to the computer with the current value of the channel and its timestamp.
    - **Disabled**. The channel enters a high impedance state. No message is accepted or generated for this channel.
  - Set the **signal** type or *protocol* of the channel:
    - **Single-ended**. +5V, +3V3 or +1V8.
    - **LVDS**.
- Command format. 8 bytes long.
  
| Field                 | Value                                                      | Type   | Byte size | Byte Offset |
|-----------------------|------------------------------------------------------------|--------|-----------|-------------|
| Start character       | `$`                                                        | `char` | 1         | 0           |
| Command descriptor    | `S`                                                        | `char` | 1         | 1           |
| Subcommand descriptor | `C`                                                        | `char` | 1         | 2           |
| Channel Number        | `00` to `99`                                               | `char` | 2         | 3           |
| Channel Mode          | `IN`: Input<br>`OU`: Output<br>`MR`: Monitor Rising edges<br>`MF`: Monitor falling edges<br>`MB`: Monitor both edges<br>`DS`: Disabled | `char` | 2         | 5           |
| Signal type           | `5`: 5V<br>`3`: 3V3<br>`1`: 1V8<br>`L`: LVDS               | `char` | 1         | 7           |

#### SYNC Settings (`SY`)

This command sets the time of reference for a SYNC pulse, its frequency and duty cycle. As previously noted, the MIDDS' time starts counting from the initialization sequence (if no SYNC is present) or from the first SYNC pulse after the initialization sequence, setting its internal time to zero on this first pulse. With this command, you may set the exact `time` of the next SYNC rising pulse. If you want to set the time of the system without use of a SYNC pulse, you may set the SYNC Channel number as -1 (frequency and duty cycles will be ignored for this command, but must fall within the allowed ranges). `time` = -1 (or `0xFFFFFFFFFFFFFFFF`) is reserved and should not be used.

- Command format. 29 bytes long.

| Field                 | Value                    | Type     | Byte size | Byte Offset |
|-----------------------|--------------------------|----------|-----------|-------------|
| Start character       | `$`                      | `char`   | 1         | 0           |
| Command descriptor    | `S`                      | `char`   | 1         | 1           |
| Subcommand descriptor | `Y`                      | `char`   | 1         | 2           |
| SYNC Channel Number   | `00` to `99` (or `-1`)   | `char`   | 2         | 3           |
| Frequency             | [00.01 Hz, 100.00 Hz]    | `double` | 8         | 5           |
| Duty cycle            | (0%, 100%)               | `double` | 8         | 13          |
| Time                  | ---                      | `time`   | 8         | 21          |

### Error Message (`E`)

This message is sent by the MIDDS when there's an internal error/warning. The message is delimited 
by a new line character `\n`. The maximum length of the message is 63 characters plus one for the
new line.

- Command format. Minimum 3 bytes, maximum 66 bytes.

| Field                 | Value                    | Type     | Byte size | Byte Offset |
|-----------------------|--------------------------|----------|-----------|-------------|
| Start character       | `$`                      | `char`   | 1         | 0           |
| Command descriptor    | `E`                      | `char`   | 1         | 1           |
| Message               | ---                      | `char[]` | n $\le$ 64    | 2           |

### Connect (`CONN`)

To start communications with MIDDS, send the `$CONN` command. MIDDS will respond with the current 
software version: `Coonected to PROTO MIDDS vx.x\n`.

### Disconnect (`DISC`)

To end communications with MIDDS, send the `$DISC` command. MIDDS will set all channels as disabled, it will send a `Disconnected\n` message and stop sending data through the serial port. 

# Hardware and Software design

See the schematics and PCB [here](/PROTO_MIDDS/hardware/rev2_0425/).

## Hardware timestamped I/Os

These I/Os are managed by a TIMx. A TIMx (normally) have four inner timers which are all fed by the general counter of the TIMx, that is, all timers inside TIMx get incremented (if enabled) at the same rate as the general counter. In the case of the MIDDS, all those inner timers will always be enabled, meaning that they will always have the same value as the general counter. Moreover, they have been configured to all be reset when the general counter overflows, guaranteeing the same frame of reference for all timers.

MIDDS uses four TIMx, which would equal to 16 timers, but due to hardware limitations, only 14 can be used. All TIMx are synchronized among each others, being TIM1 the master timer. In other words, when TIM1 overflows, it triggers the reset of TIM2, TIM3 and TIM4.

As the TIMx work at the same rate of the MCU’s clock (160 MHz), each increment of these counters will be 6.25ns.

The timers can be triggered by a rising or falling edge. When an edge is detected, the value of the timer is transferred to an inner register. Next, it triggers an interrupt so that the CPU processes the captured timer value.

If the MCU could not attend that interrupt request and another edge came, the timer would be overwritten with the time value of the newly arrived edge. That is why there is a maximum input frequency of 1 MHz on all inputs, so that no overwriting can occur.

## Software timestamped I/Os

The remaining GPIOs of the MCU can also be software timestamped.

## MCU's pinout 

The following pinout has been used on the prototype model:

|      | **PORT A**                 |     |      | **PORT B**               |
| ---  | ---                        | --- | ---  | ---                      |
| PA0  | **Ch13** (TIM2_CH1)        | /   | PB0  | **Ch02** (TIM3_CH3)      |
| PA1  | **Ch12** (TIM2_CH2)        | /   | PB1  | **Ch03** (TIM3_CH4)      |
| PA2  | **Ch11** (TIM2_CH3)        | /   | PB2  | **GPIO 03**              |
| PA3  | **Ch10** (TIM2_CH4)        | /   | PB3  | **GPIO 14**              |
| PA4  | **Ch00** (TIM3_CH2)        | /   | PB4  | **GPIO 05**              |
| PA5  | SPI (SCK)                  | /   | PB5  | **GPIO 13**              |
| PA6  | **Ch01** (TIM3_CH1)        | /   | PB6  | **Ch08** (TIM4_CH1)      |
| PA7  | SPI (MOSI)                 | /   | PB7  | **Ch09** (TIM4_CH2)      |
| PA8  | **Ch06** (TIM1_CH1)        | /   | PB8  | BOOT0                    |
| PA9  | **Ch05** (TIM1_CH2)        | /   | PB9  | **Ch07** (TIM4_CH4)      |
| PA10 | **Ch04** (TIM1_CH3)        | /   | PB10 | **GPIO 11**              |
| PA11 | USB (DM)                   | /   | PB11 | **GPIO 02**              |
| PA12 | USB (DP)                   | /   | PB12 | **GPIO 10**              |
| PA13 | Debug (SWDIO)              | /   | PB13 | **GPIO 01**              |
| PA14 | Debug (SWCLK)              | /   | PB14 | **GPIO 09**              |
| PA15 | **GPIO 07**                | /   | PB15 | **GPIO 00**              |

|      | **PORT C**                 |     |      | **Others**               |
| ---  | ---                        | --- | ---  | ---                      | 
| PC4  | **GPIO 12**                | /   | PF0  | RCC_OSC_IN               |
| PC6  | **GPIO 08**                | /   | PF1  | TFT (A0)                 |
| PC10 | **GPIO 15**                | /   | PG10 | NRST                     |
| PC11 | **GPIO 06**                | 
| PC13 | **GPIO 04**                | 
| PC14 | Shift Register (ENABLE)    |
| PC15 | TFT (Chip Select)          |
