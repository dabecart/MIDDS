/***************************************************************************************************
 * @file CircularBuffer.h
 * @brief A simple Circular or Ring buffer implementation.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef CIRCULAR_BUFFER_h
#define CIRCULAR_BUFFER_h

#include <string.h>
#include <stdint.h>

#define CIRCULAR_BUFFER_MAX_SIZE 400

typedef struct 
{
    uint8_t     data[CIRCULAR_BUFFER_MAX_SIZE]; // Data buffer.
    uint32_t    size;                           // Full size of the buffer.    
    uint32_t    len;                            // Number of bytes to read (stored bytes count).
    uint32_t    head;                           // Index to read from.
    uint32_t    tail;                           // Index to write to.
} CircularBuffer;

/**************************************** FUNCTION *************************************************
 * @brief Starts a CircularBuffer.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param bufferSize. Size of the buffer to be instantiated.
 * @return None 
***************************************************************************************************/
void init_cb(CircularBuffer* pCB, uint32_t bufferSize);

/**************************************** FUNCTION *************************************************
 * @brief Empties a CircularBuffer.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @return None 
***************************************************************************************************/
void empty_cb(CircularBuffer* pCB);

/**************************************** FUNCTION *************************************************
 * @brief Pushes a single byte into a CircularBuffer. Advances the tail index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param item. Byte to be store into the buffer.
 * @return 1 if the push was successful. 
***************************************************************************************************/
uint8_t push_cb(CircularBuffer* pCB, uint8_t item);

/**************************************** FUNCTION *************************************************
 * @brief Pushes N bytes into a CircularBuffer. Advances the tail index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param items. Bytes to be stored into the buffer.
 * @param count. Number of bytes to push.
 * @return 1 if the push was successful. 
***************************************************************************************************/
uint8_t pushN_cb(CircularBuffer* pCB, uint8_t* items, uint32_t count);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a CircularBuffer. Advances the head index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param item. Where the popped byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t pop_cb(CircularBuffer* pCB, uint8_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Reads N bytes from a CircularBuffer. Advances the head index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param count. How many bytes want to be popped.
 * @param items. Where the popped bytes will be stored. If it's NULL the indices will still be 
 * updated but no result will be returned.
 * @return 1 if the read items are valid. 
***************************************************************************************************/
uint8_t popN_cb(CircularBuffer* pCB, uint32_t count, uint8_t* items);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a CircularBuffer. Does not advance the head index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param item. Where the read byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t peek_cb(CircularBuffer* pCB, uint8_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Reads N bytes from a CircularBuffer. Does not advance the head index.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param count. How many bytes want to be peeked.
 * @param items. Where the peeked bytes will be stored.
 * @return 1 if the read items are valid. 
***************************************************************************************************/
uint8_t peekN_cb(CircularBuffer* pCB, uint32_t count, uint8_t* items);

/**************************************** FUNCTION *************************************************
 * @brief The DMA functions automatically treats a buffer as a circular buffer. The callbacks return
 * the new head of the buffer, so this function is used to update the head index accordingly.
 * @param pCB. Pointer to the CircularBuffer struct.
 * @param ullNewHeadIndex. The head index returned by the callback.
 * @return 1 if the update was OK. 
***************************************************************************************************/
uint8_t updateIndices(CircularBuffer* pCB, uint32_t ullNewHeadIndex);

#endif // CIRCULAR_BUFFER_h