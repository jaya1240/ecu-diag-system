/**
 * @file ring_buffer.c
 * @brief Implementation of a fixed-size byte FIFO. Size is a power of two
 *        so index wrap can use a mask instead of modulo (cheap on M0/M3).
 */
#include "ring_buffer.h"

#define RB_MASK (RING_BUFFER_SIZE - 1U)

void RingBuffer_Init(RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}

bool RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    return rb->head == rb->tail;
}

bool RingBuffer_IsFull(const RingBuffer_t *rb)
{
    return (uint16_t)((rb->head + 1U) & RB_MASK) == rb->tail;
}

uint16_t RingBuffer_Count(const RingBuffer_t *rb)
{
    return (uint16_t)((rb->head - rb->tail) & RB_MASK);
}

bool RingBuffer_Put(RingBuffer_t *rb, uint8_t byte)
{
    if (RingBuffer_IsFull(rb)) {
        return false; /* drop-oldest is NOT done here; caller decides policy */
    }
    rb->data[rb->head] = byte;
    rb->head = (uint16_t)((rb->head + 1U) & RB_MASK);
    return true;
}

bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *byte)
{
    if (RingBuffer_IsEmpty(rb)) {
        return false;
    }
    *byte = rb->data[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) & RB_MASK);
    return true;
}

void RingBuffer_Flush(RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}
