################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MCU/HWTimers.c \
../MCU/MainMCU.c 

OBJS += \
./MCU/HWTimers.o \
./MCU/MainMCU.o 

C_DEPS += \
./MCU/HWTimers.d \
./MCU/MainMCU.d 


# Each subdirectory must supply rules for building sources it contributes
MCU/%.o MCU/%.su MCU/%.cyclo: ../MCU/%.c MCU/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I"C:/Users/Daniel/repos/MIDDS/MCU" -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-MCU

clean-MCU:
	-$(RM) ./MCU/HWTimers.cyclo ./MCU/HWTimers.d ./MCU/HWTimers.o ./MCU/HWTimers.su ./MCU/MainMCU.cyclo ./MCU/MainMCU.d ./MCU/MainMCU.o ./MCU/MainMCU.su

.PHONY: clean-MCU

