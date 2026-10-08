/**
 * @file uart_driver.c
 * @brief Single-byte interrupt RX into a ring buffer, with line assembly
 *        done in the main loop (Uart_Driver_ReadLine). TX is done with
 *        HAL_UART_Transmit in polling mode since console output is small
 *        and infrequent — swap for DMA/IT TX if you need it non-blocking.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uart_driver.h"
#include "ring_buffer.h"

static UART_HandleTypeDef *s_huart = NULL;
static RingBuffer_t s_rx_rb;
static uint8_t s_rx_byte; /* single-byte HAL_UART_Receive_IT staging area */

static char s_line_buf[UART_LINE_MAX_LEN];
static uint16_t s_line_len = 0U;

UartStatus_t Uart_Driver_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return UART_ERR_INIT;
    }
    s_huart = huart;
    RingBuffer_Init(&s_rx_rb);
    s_line_len = 0U;

    if (HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U) != HAL_OK) {
        return UART_ERR_INIT;
    }
    return UART_OK;
}

void Uart_Driver_SendString(const char *str)
{
    if (s_huart == NULL || str == NULL) {
        return;
    }
    HAL_UART_Transmit(s_huart, (uint8_t *)str, (uint16_t)strlen(str), 100U);
}

void Uart_Driver_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Uart_Driver_SendString(buf);
}

bool Uart_Driver_ReadLine(char *line, uint16_t max_len)
{
    uint8_t byte;

    while (RingBuffer_Get(&s_rx_rb, &byte)) {
        if (byte == '\r' || byte == '\n') {
            if (s_line_len == 0U) {
                continue; /* ignore stray CR/LF (e.g. \r\n pair) */
            }
            s_line_buf[s_line_len] = '\0';
            uint16_t copy_len = (s_line_len < (max_len - 1U)) ? s_line_len : (max_len - 1U);
            memcpy(line, s_line_buf, copy_len);
            line[copy_len] = '\0';
            s_line_len = 0U;
            return true;
        }

        if (s_line_len < (UART_LINE_MAX_LEN - 1U)) {
            s_line_buf[s_line_len++] = (char)byte;
        } else {
            /* line too long: drop it and resync on the next terminator */
            s_line_len = 0U;
        }
    }
    return false;
}

/**
 * Called from HAL_UART_RxCpltCallback(). Push the received byte into the
 * ring buffer and immediately re-arm the next single-byte IT receive.
 */
void Uart_Driver_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }
    (void)RingBuffer_Put(&s_rx_rb, s_rx_byte);
    HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U);
}
