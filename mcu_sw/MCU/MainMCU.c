#include "MainMCU.h"

HWTimers hwTimers;
UART_HandleTypeDef* huart;

void initMCU(TIM_HandleTypeDef* htim1,
             TIM_HandleTypeDef* htim2, 
             TIM_HandleTypeDef* htim3, 
             TIM_HandleTypeDef* htim4,
             UART_HandleTypeDef* huart2)
{
    initHWTimers(&hwTimers, htim1, htim2, htim3, htim4);
    huart = huart2;

    startHWTimers(&hwTimers);
}

void loopMCU() {
    static char outMsg[1024];
    
    if(readyToPrintHWTimer(&hwTimers.htim1)) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim1, outMsg, sizeof(outMsg));
        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
    } 

    if(readyToPrintHWTimer(&hwTimers.htim2)) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim2, outMsg, sizeof(outMsg));
        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
    } 

    if(readyToPrintHWTimer(&hwTimers.htim3)) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim3, outMsg, sizeof(outMsg));
        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
    } 

    if(readyToPrintHWTimer(&hwTimers.htim4)) {
        uint16_t msgSize = printHWTimer(&hwTimers.htim4, outMsg, sizeof(outMsg));
        HAL_UART_Transmit(huart, (uint8_t*) outMsg, msgSize, 1000);
    } 
}
