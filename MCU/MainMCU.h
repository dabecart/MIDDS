#ifndef MAIN_MCU_h
#define MAIN_MCU_h

#include "main.h"
#include "HWTimers.h"
#include "stm32g4xx_hal.h"

#define TX_TIME_INTERVAL 10 // ms

void initMCU(TIM_HandleTypeDef* htim1, UART_HandleTypeDef* huart2);

void loopMCU();

extern HWTimers hwTimers;

#endif // MAIN_MCU_h