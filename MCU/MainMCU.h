#ifndef MAIN_MCU_h
#define MAIN_MCU_h

#include "main.h"
#include "HWTimers.h"
#include "stm32g4xx_hal.h"

#define MCU_TX_IN_ASCII 0

void initMCU(TIM_HandleTypeDef* htim1, UART_HandleTypeDef* huart2);

void loopMCU();

extern HWTimers hwTimers;

#endif // MAIN_MCU_h