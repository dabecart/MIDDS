/***************************************************************************************************
 * @file CircularBuffer64.h
 * @brief A simple Circular or Ring buffer implementation for 64 bit data.
 * 
 * @project MIDDS
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef CIRCULAR_BUFFER_64_h
#define CIRCULAR_BUFFER_64_h

#include <string.h>
#include <stdint.h>

#define CIRCULAR_BUFFER_64_MAX_SIZE 200

typedef struct 
{
    uint32_t    size;                               // Full size of the buffer.    
    uint32_t    len;                                // Number of bytes to read (stored bytes count).
    uint32_t    head;                               // Index to read from.
    uint32_t    tail;                               // Index to write to.
    uint64_t    data[CIRCULAR_BUFFER_64_MAX_SIZE];  // Data buffer.
    uint8_t     locked;                             // 1 to disable pushing into the buffer.
} __attribute__((__packed__)) CircularBuffer64;

/**************************************** FUNCTION *************************************************
 * @brief Starts a CircularBuffer64.
 * @param pCB. Pointer to the CircularBuffer64 struct.
 * @param bufferSize. Size of the buffer to be instantiated.
 * @return None 
***************************************************************************************************/
void init_cb64(CircularBuffer64* pCB, uint32_t bufferSize);

/**************************************** FUNCTION *************************************************
 * @brief Empties the content of a cb64.
 * @param pCB. Pointer to the CircularBuffer64 struct.
***************************************************************************************************/
void empty_cb64(CircularBuffer64* pCB);

/**************************************** FUNCTION *************************************************
 * @brief Pushes a single byte into a CircularBuffer64. Advances the tail index.
 * @param pCB. Pointer to the CircularBuffer64 struct.
 * @param item. Byte to be store into the buffer.
 * @return 1 if the push was successful. 
***************************************************************************************************/
uint8_t push_cb64(CircularBuffer64* pCB, uint64_t item);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a CircularBuffer64. Advances the head index.
 * @param pCB. Pointer to the CircularBuffer64 struct.
 * @param item. Where the popped byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t pop_cb64(CircularBuffer64* pCB, uint64_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a CircularBuffer64. Does not advance the head index.
 * @param pCB. Pointer to the CircularBuffer64 struct.
 * @param item. Where the read byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t peek_cb64(CircularBuffer64* pCB, uint64_t* item);

#endif // CIRCULAR_BUFFER_64_h