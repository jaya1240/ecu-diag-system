/**
 * @file    uart_driver.h
 * @brief   Interrupt-driven UART console: byte-at-a-time RX into a ring
 *          buffer, line assembly, and blocking-ish TX helpers for the
 *          bench diagnostic console (115200 8N1).
 */
#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_LINE_MAX_LEN 96U

typedef enum {
    UART_OK = 0,
    UART_ERR_INIT,
    UART_ERR_NO_LINE,
    UART_ERR_OVERFLOW
} UartStatus_t;

UartStatus_t Uart_Driver_Init(UART_HandleTypeDef *huart);
void         Uart_Driver_SendString(const char *str);
void         Uart_Driver_Printf(const char *fmt, ...);

/* Returns true and fills `line` (NUL-terminated) once a full line
 * (terminated by \r or \n) has been received. Non-blocking. */
bool Uart_Driver_ReadLine(char *line, uint16_t max_len);

/* Called from HAL_UART_RxCpltCallback in main.c / stm32*_it.c */
void Uart_Driver_RxCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
