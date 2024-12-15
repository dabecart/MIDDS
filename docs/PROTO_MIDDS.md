# MIDDS Prototype
*Monitoring Interface of Digital and Differential Signals, by @dabecart.*

There are two versions of the MIDDS in this project: 
- The **prototype** model: easy to build with a prototype board, based around the **STM32G431** MCU, in particular the [WeActStudio STM32G431 Core Board](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard) This was used as a proof of concept for the final model.
- The **final** model: implemented in a PCB with the **STM32H753ZIT6** MCU in mind, in particular the **NUCLEO-H753ZI** board.

In this file, some of the secChns from the [README of the docs secChn](docs\README.md) will be applied to the prototype version of the MIDDS. The working principle of the MIDDS and its communicaChn protocol remains unchanged. The main differences between boards will arise from the hardware limitaChns of the far simpler MCU and the external hardware found on the PCB of the final model.

# Hardware differences between the final and prototype MIDDS

Regarding the MCUs:

- **GPIOs**. The prototype MCU has far less GPIOs, having exposed all pins of GPIO channel A and B and some of C.
- **HRTIM**. The prototype MCU lacks an HRTIM. Therefore, TIM1 acts as the HRTIM or master timer of the MIDDS. 
- **Ethernet**. The prototype MCU does not have ethernet capabilities, so a UART port is used to communicate with the computer. During development, a UART to USB converter based on CH340 was used, with 921600 baudrate.
- **Clock frequency**. The maximum speed of the prototype MCU's HCLK is 170 MHz, so expect less processing capabilities compared to the final MCU and time precision in the measurements. In the proto, HCLK = 160 MHz, therefore each step in the fine time counter is 6.25 ns. 

Regarding external hardware:
- **LVDS support**. The chips used to convert from LVDS to TTL and vice versa are SMD, and although a socket could be used to convert it from SMD to TTL and plug it in the protoboard, it is rather inconvenient so this has been dropped from the prototype version.

The following pinout has been used on the prototype model:

|      | **PORT A**                 |     |      | **PORT B**               |
| ---  | ---                        | --- | ---  | ---                      |
| PA0  | **Ch04** (TIM2_CH1)        | /   | PB0  | **Ch10** (TIM3_CH3)      |
| PA1  | **Ch05** (TIM2_CH2)        | /   | PB1  | **Ch11** (TIM3_CH4)      |
| PA2  | UART TX (USART2_TX)        | /   | PB2  |                          |
| PA3  | UART RX (USART2_RX)        | /   | PB3  |                          |
| PA4  | **Ch09** (TIM3_CH2)        | /   | PB4  |                          |
| PA5  |                            | /   | PB5  |                          |
| PA6  | **Ch08** (TIM3_CH1)        | /   | PB6  | **Ch12** (TIM4_CH1)      |
| PA7  |                            | /   | PB7  |                          |
| PA8  | **Ch00** (TIM1_CH1)        | /   | PB8  |                          |
| PA9  | **Ch01** (TIM1_CH2)        | /   | PB9  | **Ch14** (TIM4_CH4)      |
| PA10 | **Ch02** (TIM1_CH3)        | /   | PB10 | **Ch06** (TIM2_CH3)      |
| PA11 | **Ch03** (TIM1_CH4)        | /   | PB11 | **Ch07** (TIM2_CH4)      |
| PA12 | **Ch13** (TIM4_CH2)        | /   | PB12 |                          |
| PA13 | Debug (SWDIO)              | /   | PB13 |                          |
| PA14 | Debug (SWCLK)              | /   | PB14 |                          |
| PA15 |                            | /   | PB15 |                          |

|      | **PORT C**                 |
| ---  | ---                        | 
| PC4  |                            |
| PC6  |                            |
| PC10 |                            |
| PC11 |                            |
| PC13 |                            |