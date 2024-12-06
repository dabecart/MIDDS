#include "CircularBuffer.h"

void init_cb(CircularBuffer* pstCB, uint32_t bufferSize) {
    if(pstCB == NULL) return;

    pstCB->size = bufferSize;
    pstCB->len  = 0;
    pstCB->head = 0;
    pstCB->tail = 0;
}

void empty_cb(CircularBuffer* pstCB) {
    if(pstCB == NULL) return;

    pstCB->head = 0;
    pstCB->tail = 0;
    pstCB->len  = 0;
    memset(pstCB->data, 0, pstCB->size);
}

uint8_t push_cb(CircularBuffer* pstCB, uint8_t ucItem) {
    if((pstCB->len+1) >= pstCB->size) return 0;

    pstCB->data[pstCB->head] = ucItem;
    pstCB->head = (pstCB->head + 1) % pstCB->size;
    pstCB->len++; 
    return 1;
}

uint8_t pushN_cb(CircularBuffer* pstCB, uint8_t* items, uint32_t count) {
    if(items == NULL) return 0;

    if((pstCB->len + count) >= pstCB->size) return 0;
    
    uint32_t ullNextHead = pstCB->head + count;
    if(ullNextHead > pstCB->size) {
        uint32_t ullHeadBytes = pstCB->size - pstCB->head;
        memcpy(pstCB->data+pstCB->head, items, ullHeadBytes);
        memcpy(pstCB->data, items + ullHeadBytes, count - ullHeadBytes);
    }else {
        memcpy(pstCB->data+pstCB->head, items, count);
    }

    pstCB->head = ullNextHead % pstCB->size;
    pstCB->len += count; 
    return 1;
}

uint8_t pop_cb(CircularBuffer* pstCB, uint8_t* item) {
    if(pstCB->len < 1) return 0;

    *item = pstCB->data[pstCB->tail];
    pstCB->tail = (pstCB->tail + 1) % pstCB->size;
    pstCB->len--;
    return 1;
}

uint8_t popN_cb(CircularBuffer* pstCB, uint32_t count, uint8_t* items) {
    if(pstCB->len < count) return 0;
    
    uint32_t ullNextTail = pstCB->tail + count;
    if(items != NULL) {
        if(ullNextTail > pstCB->size) {
            uint32_t ullTailBytes = pstCB->size-pstCB->tail;
            memcpy(items, pstCB->data+pstCB->tail, ullTailBytes);
            memcpy(items + ullTailBytes, pstCB->data, count - ullTailBytes);
        }else {
            memcpy(items, pstCB->data + pstCB->tail, count);
        }
    }

    pstCB->tail = ullNextTail % pstCB->size;
    pstCB->len -= count;
    return 1;
}

uint8_t peek_cb(CircularBuffer* pstCB, uint8_t* item) {
    if(pstCB->len < 1) return 0;
    
    *item = pstCB->data[pstCB->tail];
    return 1;
}

uint8_t peekN_cb(CircularBuffer* pstCB, uint32_t count, uint8_t* items) {
    if(items == NULL) return 0;

    if(pstCB->len < count) return 0;
    
    uint32_t ullNextTail = pstCB->tail + count;
    if(ullNextTail > pstCB->size) {
        uint32_t ullTailBytes = pstCB->size-pstCB->tail;
        memcpy(items, pstCB->data+pstCB->tail, ullTailBytes);
        memcpy(items + ullTailBytes, pstCB->data, count - ullTailBytes);
    }else {
        memcpy(items, pstCB->data + pstCB->tail, count);
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
