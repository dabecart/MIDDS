#ifndef CIRCULAR_BUFFER_64_h
#define CIRCULAR_BUFFER_64_h

#include <string.h>
#include <stdint.h>

#define CIRCULAR_BUFFER_64_MAX_SIZE 128

typedef struct 
{
    uint64_t    data[CIRCULAR_BUFFER_64_MAX_SIZE]; // Data buffer.
    uint32_t    size;                           // Full size of the buffer.    
    uint32_t    len;                            // Number of bytes to read (stored bytes count).
    uint32_t    head;                           // Index to read from.
    uint32_t    tail;                           // Index to write to.
} CircularBuffer64;

/**************************************** FUNCTION *************************************************
 * \brief Starts a CircularBuffer.
 * \param pCB. Pointer to the CircularBuffer struct.
 * \param bufferSize. Size of the buffer to be instantiated.
 * \return None 
***************************************************************************************************/
void init_cb64(CircularBuffer64* pCB, uint32_t bufferSize);

/**************************************** FUNCTION *************************************************
 * \brief Pushes a single byte into a CircularBuffer. Advances the tail index.
 * \param pCB. Pointer to the CircularBuffer struct.
 * \param item. Byte to be store into the buffer.
 * \return TRUE if the push was successful. 
***************************************************************************************************/
uint8_t push_cb64(CircularBuffer64* pCB, uint64_t item);

/**************************************** FUNCTION *************************************************
 * \brief Reads a byte from a CircularBuffer. Advances the head index.
 * \param pCB. Pointer to the CircularBuffer struct.
 * \param item. Where the popped byte will be stored.
 * \return uint8_t TRUE if the read item is valid. 
***************************************************************************************************/
uint8_t pop_cb64(CircularBuffer64* pCB, uint64_t* item);

#endif // CIRCULAR_BUFFER_64_h