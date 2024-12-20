# MIDDS Prototype
*Monitoring Interface of Digital and Differential Signals, by @dabecart.*

There are two versions of the MIDDS in this project: 
- The **prototype** model: easy to build with a prototype board, based around the **STM32G431** MCU, in particular the [WeActStudio STM32G431 Core Board](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard) This was used as a proof of concept for the final model.
- The **final** model: implemented in a PCB with the **STM32H753ZIT6** MCU in mind, in particular the **NUCLEO-H753ZI** board.

In this file, some of the sections from the [README](docs\README.md) will be applied to the prototype version of the MIDDS. The working principle of the MIDDS and its communication protocol remains unchanged. The main differences between boards will arise from the hardware limitations of the far simpler MCU and the external hardware found on the PCB of the final model.

# Hardware differences between the final and prototype MIDDS

Regarding the MCUs:

- **GPIOs**. The prototype MCU has far less GPIOs, having exposed all pins of GPIO channel A and B and some of C.
- **No HRTIM**. The prototype MCU lacks an HRTIM. Therefore, TIM1 acts as the HRTIM or master timer of the MIDDS. 
- **USB instead of Ethernet**. The prototype MCU does not have ethernet capabilities, so the CDC USB is used to communicate with the computer.
- **Clock frequency**. The maximum speed of the prototype MCU's HCLK is 170 MHz, so expect less processing capabilities compared to the final MCU and time precision in the measurements. In the proto, HCLK = 160 MHz, therefore each step in the fine time counter is 6.25 ns. 

The following pinout has been used on the prototype model:

|      | **PORT A**                 |     |      | **PORT B**               |
| ---  | ---                        | --- | ---  | ---                      |
| PA0  | **Ch03** (TIM2_CH1)        | /   | PB0  | **Ch09** (TIM3_CH3)      |
| PA1  | **Ch04** (TIM2_CH2)        | /   | PB1  | **Ch10** (TIM3_CH4)      |
| PA2  | **Ch05** (TIM2_CH3)        | /   | PB2  |                          |
| PA3  | **Ch06** (TIM2_CH4)        | /   | PB3  |                          |
| PA4  | **Ch08** (TIM3_CH2)        | /   | PB4  |                          |
| PA5  | SPI (SCK)                  | /   | PB5  |                          |
| PA6  | **Ch07** (TIM3_CH1)        | /   | PB6  | **Ch11** (TIM4_CH1)      |
| PA7  | SPI (MOSI)                 | /   | PB7  | **Ch12** (TIM4_CH2)      |
| PA8  | **Ch00** (TIM1_CH1)        | /   | PB8  |                          |
| PA9  | **Ch01** (TIM1_CH2)        | /   | PB9  | **Ch13** (TIM4_CH4)      |
| PA10 | **Ch02** (TIM1_CH3)        | /   | PB10 |                          |
| PA11 | USB (DM)                   | /   | PB11 |                          |
| PA12 | USB (DP)                   | /   | PB12 |                          |
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