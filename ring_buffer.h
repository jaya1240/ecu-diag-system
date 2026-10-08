/**
 * @file    ring_buffer.h
 * @brief   Small lock-free (single-producer/single-consumer) byte FIFO
 *          used by both the UART RX path and the CAN RX queue.
 */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RING_BUFFER_SIZE  256U   /* must be a power of two */

typedef struct {
    uint8_t  data[RING_BUFFER_SIZE];
    volatile uint16_t head;      /* write index (ISR)   */
    volatile uint16_t tail;      /* read index (main)   */
} RingBuffer_t;

void     RingBuffer_Init(RingBuffer_t *rb);
bool     RingBuffer_Put(RingBuffer_t *rb, uint8_t byte);
bool     RingBuffer_Get(RingBuffer_t *rb, uint8_t *byte);
uint16_t RingBuffer_Count(const RingBuffer_t *rb);
bool     RingBuffer_IsEmpty(const RingBuffer_t *rb);
bool     RingBuffer_IsFull(const RingBuffer_t *rb);
void     RingBuffer_Flush(RingBuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
