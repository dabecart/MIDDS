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
    static char outMsg[1024];
    
    if(readyToPrintHWTimer(&hwTimers.htim1)) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim1, outMsg, sizeof(outMsg));
        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
    } 
}
