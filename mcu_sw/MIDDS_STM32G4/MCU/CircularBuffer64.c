/***************************************************************************************************
 * @file CircularBuffer64.c
 * @brief A simple Circular or Ring buffer implementation for 64 bit data.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "CircularBuffer64.h"

void init_cb64(CircularBuffer64* pstCB, uint32_t bufferSize) {
    if(pstCB == NULL) return;

    pstCB->size = bufferSize;
    pstCB->len  = 0;
    pstCB->head = 0;
    pstCB->tail = 0;
}

uint8_t push_cb64(CircularBuffer64* pstCB, uint64_t ucItem) {
    if((pstCB->len+1) >= pstCB->size) return 0;

    pstCB->data[pstCB->head] = ucItem;
    pstCB->head = (pstCB->head + 1) % pstCB->size;
    pstCB->len++; 
    return 1;
}

uint8_t pop_cb64(CircularBuffer64* pstCB, uint64_t* item) {
    if(pstCB->len < 1) return 0;

    *item = pstCB->data[pstCB->tail];
    pstCB->tail = (pstCB->tail + 1) % pstCB->size;
    pstCB->len--;
    return 1;
}
