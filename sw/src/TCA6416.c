/***************************************************************************************************
 * @file TCA6416.c
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

#include "TCA6416.h"

uint8_t initGPIOExpander(GPIOExpander* gpio, I2C_HandleTypeDef* i2cHandler, uint8_t i2cAddress) {
    if(gpio == NULL || i2cHandler == NULL) return 0;

    gpio->i2cHandler = i2cHandler;
    gpio->i2cAddrs = i2cAddress << 1UL;
    gpio->initialized = 1;
    
    // Read the initial direction of the device.
    uint8_t directionLow = 0, directionHigh = 0;
    uint8_t status = readGPIOExpanderRegister_(gpio, TCA6416_CONFIGURATION_0, &directionLow);
    status &= readGPIOExpanderRegister_(gpio, TCA6416_CONFIGURATION_1, &directionHigh);
    gpio->direction = (directionHigh << 8) | directionLow;

    // Disable the inverse polarity on all registers.
    status &= writeGPIOExpanderRegister_(gpio, TCA6416_POLARITY_INVERSION_0, 0);
    status &= writeGPIOExpanderRegister_(gpio, TCA6416_POLARITY_INVERSION_1, 0);

    gpio->initialized = status;
    return status;
}

uint8_t setDirectionGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_Direction dir) {
    if(gpio == NULL || !gpio->initialized || 
       (dir != GPIOEx_Output && dir != GPIOEx_Input) || pin >= TCA6416_GPIO_COUNT) {
        return 0;
    } 
        
    uint16_t newDirection = (gpio->direction & ~(1UL << pin)) | (dir << pin);
    if(newDirection == gpio->direction) {
        // Nothing to do here...
        return 1;
    }

    TCA6416Registers reg;
    uint8_t newDirectionRegValue = 0;
    if(pin < 8) {
        reg = TCA6416_CONFIGURATION_0;
        newDirectionRegValue = newDirection & 0xFF;
    }else {
        reg = TCA6416_CONFIGURATION_1;
        newDirectionRegValue = (newDirection >> 8) & 0xFF;
    }
    uint8_t status = writeGPIOExpanderRegister_(gpio, reg, newDirectionRegValue);
    
    uint8_t readDirRegister = 0;
    status &= readGPIOExpanderRegister_(gpio, reg, &readDirRegister);
    
    status &= (newDirectionRegValue == readDirRegister);

    if(status) {
        gpio->direction = newDirection;
    }
    return status;
}

uint8_t setStateGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_State state) {
    if(gpio == NULL || !gpio->initialized || pin >= TCA6416_GPIO_COUNT) return 0;

    // if(((gpio->direction >> pin) & 0x01) != GPIOEx_Output) {
    //     // The pin is not configured as an output.
    //     return 0;
    // }

    TCA6416Registers writeReg;
    if(pin < 8) {
        writeReg = TCA6416_OUTPUT_PORT_0;
    }else {
        writeReg = TCA6416_OUTPUT_PORT_1;
        pin -= 8;
    }

    uint8_t writeRegContent = 0;
    uint8_t status = readGPIOExpanderRegister_(gpio, writeReg, &writeRegContent);

    // New register content with the state of the pin.
    writeRegContent = (writeRegContent & ~(1UL << pin)) | (state << pin);

    status &= writeGPIOExpanderRegister_(gpio, writeReg, writeRegContent);

    // Read the register and check that it was properly configured.
    uint8_t finalRead = 0;
    status &= readGPIOExpanderRegister_(gpio, writeReg, &finalRead);
    status &= (finalRead == writeRegContent);

    return status;
}

uint8_t getStateGPIOExpander(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state) {
    if(gpio == NULL || state == NULL || !gpio->initialized || pin >= TCA6416_GPIO_COUNT) {
        return 0;
    }

    TCA6416Registers readReg;
    if(pin < 8) {
        readReg = TCA6416_INPUT_PORT_0;
    }else {
        readReg = TCA6416_INPUT_PORT_1;
        pin -= 8;
    }

    uint8_t readRegContent = 0;
    uint8_t status = readGPIOExpanderRegister_(gpio, readReg, &readRegContent);

    if(status) {
        if(((readRegContent >> pin) & 0x01) == GPIOEx_HIGH) {
            *state = GPIOEx_HIGH;
        }else {
            *state = GPIOEx_LOW;
        }
    }

    return status;
}

uint8_t getStateGPIOExpanderFromDMA(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state) {
    if(gpio == NULL || state == NULL || !gpio->initialized || pin >= TCA6416_GPIO_COUNT) {
        return 0;
    }

    uint8_t readRegContent = 0;
    if(pin < 8) {
        readRegContent = gpio->dmaInputPort0;
    }else {
        readRegContent = gpio->dmaInputPort1;
        pin -= 8;
    }

    if(((readRegContent >> pin) & 0x01) == GPIOEx_HIGH) {
        *state = GPIOEx_HIGH;
    }else {
        *state = GPIOEx_LOW;
    }

    return 1;
}

uint8_t getStateGPIOExpanderFromPolling(GPIOExpander* gpio, uint8_t pin, GPIOEx_State* state) {
    if(gpio == NULL || state == NULL || !gpio->initialized || pin >= TCA6416_GPIO_COUNT) {
        return 0;
    }

    uint8_t readRegContent = 0;
    if(pin < 8) {
        readRegContent = gpio->inputPort0;
    }else {
        readRegContent = gpio->inputPort1;
        pin -= 8;
    }

    if(((readRegContent >> pin) & 0x01) == GPIOEx_HIGH) {
        *state = GPIOEx_HIGH;
    }else {
        *state = GPIOEx_LOW;
    }

    return 1;
}

uint8_t writeGPIOExpanderRegister_(GPIOExpander* gpio, TCA6416Registers reg, uint8_t value) {
    if(gpio == NULL || !gpio->initialized) return 0;
    
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(
        gpio->i2cHandler, gpio->i2cAddrs, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);
    
    return st == HAL_OK;
}

uint8_t readGPIOExpanderRegister_(GPIOExpander* gpio, TCA6416Registers reg, uint8_t* value) {
    if(gpio == NULL || value == NULL || !gpio->initialized) return 0;
    
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
        gpio->i2cHandler, gpio->i2cAddrs, reg, I2C_MEMADD_SIZE_8BIT, value, 1, 1000);
    
    return st == HAL_OK;
}

uint8_t readGPIOExpanderRegisterDMA_(GPIOExpander* gpio) {
    if(gpio == NULL || !gpio->initialized) return 0;
    
    // This will read the two Input Port registers and store them into dmaInputPortx of gpio. 
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read_DMA(
        gpio->i2cHandler, gpio->i2cAddrs, TCA6416_INPUT_PORT_0, 
        I2C_MEMADD_SIZE_8BIT, &gpio->dmaInputPort0, 2);
    
    return st == HAL_OK;
}

uint8_t readGPIOExpanderRegisterPolling_(GPIOExpander* gpio) {
    if(gpio == NULL || !gpio->initialized) return 0;
    
    // This will read the two Input Port registers and store them into inputPortx of gpio. 
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
        gpio->i2cHandler, gpio->i2cAddrs, TCA6416_INPUT_PORT_0, 
        I2C_MEMADD_SIZE_8BIT, &gpio->inputPort0, 2, 1000);
    
    return st == HAL_OK;
}
