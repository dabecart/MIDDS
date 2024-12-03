#include "MainMCU.h"

HWTimers hwTimers;
UART_HandleTypeDef* huart;

void initMCU(TIM_HandleTypeDef* htim1, UART_HandleTypeDef* huart2) {
    initHWTimers(htim1, &hwTimers);
    huart = huart2;

    startHWTimers(&hwTimers);

    const uint8_t initSerialPort[] = "START";
    HAL_UART_Transmit(huart, initSerialPort, sizeof(initSerialPort), 1000);
}

void loopMCU() {
    static uint32_t loopStart = 0;
    static char outMsg[512];
    
    if((HAL_GetTick() - loopStart) > TX_TIME_INTERVAL) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim1, outMsg, sizeof(outMsg));

        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
        
        loopStart = HAL_GetTick();
    } 
}