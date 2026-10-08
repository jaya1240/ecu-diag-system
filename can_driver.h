/**
 * @file    can_driver.h
 * @brief   Thin wrapper around the STM32 bxCAN HAL for diagnostic
 *          request/response traffic on a single standard-ID pair.
 */
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_DIAG_REQUEST_ID      0x7E0U   /* tester -> ECU  (physical addr) */
#define CAN_DIAG_RESPONSE_ID     0x7E8U   /* ECU -> tester                  */
#define CAN_MAX_DLC              8U

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[CAN_MAX_DLC];
} CanFrame_t;

/* Result of Can_Driver_Init / send calls */
typedef enum {
    CAN_OK = 0,
    CAN_ERR_INIT,
    CAN_ERR_TX_BUSY,
    CAN_ERR_NO_DATA,
    CAN_ERR_FILTER
} CanStatus_t;

CanStatus_t Can_Driver_Init(CAN_HandleTypeDef *hcan);
CanStatus_t Can_Driver_Send(const CanFrame_t *frame);
bool        Can_Driver_Available(void);
CanStatus_t Can_Driver_Receive(CanFrame_t *frame);

/* Called from HAL_CAN_RxFifo0MsgPendingCallback in main.c / stm32*_it.c */
void Can_Driver_RxCallback(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* CAN_DRIVER_H */
