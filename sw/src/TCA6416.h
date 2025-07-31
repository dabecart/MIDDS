/***************************************************************************************************
 * @file TCA6416.h
 * @brief Library to control a TCA6416A: Low-Voltage 16-Bit I2C and SMBus I/O Expander With Voltage
 * Translation, Interrupt Output, Reset Input, and Configuration Registers.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2025-07-29
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef TCA6416_h
#define TCA6416_h

#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"

#define TCA6416_INITIAL_DIRECTION 0xFFFF
#define TCA6416_GPIO_COUNT 16

typedef enum TCA6416Registers {
    TCA6416_INPUT_PORT_0 = 0,
    TCA6416_INPUT_PORT_1,
    TCA6416_OUTPUT_PORT_0,
    TCA6416_OUTPUT_PORT_1,
    TCA6416_POLARITY_INVERSION_0,
    TCA6416_POLARITY_INVERSION_1,
    TCA6416_CONFIGURATION_0,
    TCA6416_CONFIGURATION_1
} TCA6416Registers;

typedef enum GPIOEx_State {
    GPIOEx_LOW = 0,
    GPIOEx_HIGH = 1,
} GPIOEx_State;

typedef enum GPIOEx_Direction {
    GPIOEx_Output = 0,
    GPIOEx_Input = 1,
} GPIOEx_Direction;

typedef struct GPIOExpander {
    I2C_HandleTypeDef* i2cHandler;
    uint8_t     i2cAddrs;
    uint8_t     initialized;
    uint16_t    direction; // 0: Output pin, 1: Input pin

    // Read all inputs after a call to readGPIOExpanderRegisterPolling_.
    uint8_t inputPort0;
    uint8_t inputPort1;

    // DMA stores the read inputs after a call to readGPIOExpanderRegisterDMA_.
    uint8_t     dmaInputPort0;
    uint8_t     dmaInputPort1;
} __attribute__((__packed__)) GPIOExpander;

uint8_t initGPIOExpander(GPIOExpander* gpio, I2C_HandleTypeDef* i2cHandler, uint8_t i2cAddress);

uint8_t setDirectionGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_Direction dir);

uint8_t setStateGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_State state);
uint8_t getStateGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state);
uint8_t getStateGPIOExpanderFromPolling(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state);
uint8_t getStateGPIOExpanderFromDMA(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state);

uint8_t writeGPIOExpanderRegister_(GPIOExpander* gpio, TCA6416Registers reg, uint8_t value);
uint8_t readGPIOExpanderRegister_(GPIOExpander* gpio, TCA6416Registers reg, uint8_t* value);
uint8_t readGPIOExpanderRegisterDMA_(GPIOExpander* gpio);
uint8_t readGPIOExpanderRegisterPolling_(GPIOExpander* gpio);

#endif // TCA6416_h