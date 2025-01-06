# PROTO MIDDS
*Monitoring Interface of Digital and Differential Signals (Prototype Model), by @dabecart.*

There are two versions of the MIDDS in this project: 
- The **prototype** model (called **PROTO MIDDS**): easy to build with a prototype board, based around the **STM32G431** MCU, in particular the [WeActStudio STM32G431 Core Board](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard) This was used as a proof of concept for the final model.
- The **final** model: implemented in a PCB with the **STM32H753ZIT6** MCU in mind, in particular the **NUCLEO-H753ZI** board.

In this file, some of the sections from the [README](docs\README.md) will be applied to the prototype version of the MIDDS. The working principle of the MIDDS and its communication protocol remains unchanged. The main differences between boards will arise from the hardware limitations of the far simpler MCU and the external hardware found on the PCB of the final model.

# Hardware differences between the MIDDS and PROTO MIDDS

Regarding the MCUs:

- **GPIOs**. The prototype MCU has far less GPIOs, having exposed all pins of GPIO channel A and B and some of C.
- **No HRTIM**. The prototype MCU lacks an HRTIM. Therefore, TIM1 acts as the HRTIM or master timer of the MIDDS. 
- **USB instead of Ethernet**. The prototype MCU does not have ethernet capabilities, so the CDC USB is used to communicate with the computer.
- **Clock frequency**. The maximum speed of the prototype MCU's HCLK is 170 MHz, so expect less processing capabilities compared to the final MCU and time precision in the measurements. In the proto, HCLK = 160 MHz, therefore each step in the fine time counter is 6.25 ns. 

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