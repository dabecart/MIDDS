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

void init_cb64(CircularBuffer64* pCB, uint32_t bufferSize) {
    if(pCB == NULL) return;

    pCB->size = bufferSize;
    pCB->len  = 0;
    pCB->head = 0;
    pCB->tail = 0;
}

uint8_t push_cb64(CircularBuffer64* pCB, uint64_t ucItem) {
    if((pCB->len+1) >= pCB->size) return 0;

    pCB->data[pCB->head] = ucItem;
    pCB->head = (pCB->head + 1) % pCB->size;
    pCB->len++; 
    return 1;
}

uint8_t pop_cb64(CircularBuffer64* pCB, uint64_t* item) {
    if(pCB->len < 1) return 0;

    *item = pCB->data[pCB->tail];
    pCB->tail = (pCB->tail + 1) % pCB->size;
    pCB->len--;
    return 1;
}

uint8_t peek_cb64(CircularBuffer64* pCB, uint64_t* item) {
    if(pCB->len < 1) return 0;
    
    *item = pCB->data[pCB->tail];
    return 1;
}

void empty_cb64(CircularBuffer64* pCB) {
    pCB->tail = pCB->head;
    pCB->len = 0;
}
