/***************************************************************************************************
 * @file CircularBuffer.c
 * @brief A simple Circular or Ring buffer implementation.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "CircularBuffer.h"

void init_cb(CircularBuffer* pCB, uint32_t bufferSize) {
    if(pCB == NULL) return;

    pCB->size = bufferSize;
    pCB->len  = 0;
    pCB->head = 0;
    pCB->tail = 0;
}

void empty_cb(CircularBuffer* pCB) {
    if(pCB == NULL) return;

    pCB->head = 0;
    pCB->tail = 0;
    pCB->len  = 0;
    memset(pCB->data, 0, pCB->size);
}

uint8_t push_cb(CircularBuffer* pCB, uint8_t ucItem) {
    if(pCB->len >= pCB->size) return 0;

    pCB->data[pCB->head] = ucItem;
    pCB->head = (pCB->head + 1) % pCB->size;
    pCB->len++; 
    return 1;
}

uint8_t pushN_cb(CircularBuffer* pCB, uint8_t* items, uint32_t count) {
    if(items == NULL) return 0;

    if((pCB->len + count) > pCB->size) return 0;
    
    uint32_t ullNextHead = pCB->head + count;
    if(ullNextHead > pCB->size) {
        uint32_t ullHeadBytes = pCB->size - pCB->head;
        memcpy(pCB->data+pCB->head, items, ullHeadBytes);
        memcpy(pCB->data, items + ullHeadBytes, count - ullHeadBytes);
    }else {
        memcpy(pCB->data+pCB->head, items, count);
    }

    pCB->head = ullNextHead % pCB->size;
    pCB->len += count; 
    return 1;
}

uint8_t pop_cb(CircularBuffer* pCB, uint8_t* item) {
    if(pCB->len < 1) return 0;

    *item = pCB->data[pCB->tail];
    pCB->tail = (pCB->tail + 1) % pCB->size;
    pCB->len--;
    return 1;
}

uint8_t popN_cb(CircularBuffer* pCB, uint32_t count, uint8_t* items) {
    if(pCB->len < count) return 0;
    
    uint32_t nextTail = pCB->tail + count;
    if(items != NULL) {
        if(nextTail > pCB->size) {
            uint32_t ullTailBytes = pCB->size-pCB->tail;
            memcpy(items, pCB->data+pCB->tail, ullTailBytes);
            memcpy(items + ullTailBytes, pCB->data, count - ullTailBytes);
        }else {
            memcpy(items, pCB->data + pCB->tail, count);
        }
    }

    pCB->tail = nextTail % pCB->size;
    pCB->len -= count;
    return 1;
}

uint8_t peek_cb(CircularBuffer* pCB, uint8_t* item) {
    if(pCB->len < 1) return 0;
    
    *item = pCB->data[pCB->tail];
    return 1;
}

uint8_t peekN_cb(CircularBuffer* pCB, uint32_t count, uint8_t* items) {
    if(items == NULL) return 0;

    if(pCB->len < count) return 0;
    
    uint32_t nextTail = pCB->tail + count;
    if(nextTail > pCB->size) {
        uint32_t ullTailBytes = pCB->size-pCB->tail;
        memcpy(items, pCB->data+pCB->tail, ullTailBytes);
        memcpy(items + ullTailBytes, pCB->data, count - ullTailBytes);
    }else {
        memcpy(items, pCB->data + pCB->tail, count);
    }

    return 1;
}

uint8_t updateIndices(CircularBuffer* pCB, uint32_t ullNewHeadIndex)
{
    uint32_t ullReadBytes = 0;
    if(ullNewHeadIndex >= pCB->head) {
        ullReadBytes = ullNewHeadIndex - pCB->head;
    }else {
        ullReadBytes = ullNewHeadIndex + pCB->size - pCB->head;
    }

    // Is data being overwritten without being processed? 
    if((ullReadBytes + pCB->len) > pCB->size) {
        // Update the tail index too.
        pCB->tail += ullReadBytes - pCB->len;
        pCB->tail %= pCB->size;
        pCB->len = pCB->size;
    }else {
        pCB->len += ullReadBytes;
    }

    pCB->head = ullNewHeadIndex;

    return 1;
}
