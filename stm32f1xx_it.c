/**
 * @file stm32f1xx_it.c
 * @brief Core exception handlers + peripheral IRQ handlers. Peripheral IRQs
 *        just forward into HAL_*_IRQHandler(), which in turn invokes the
 *        callbacks implemented in main.c (Can_Driver_RxCallback via
 *        HAL_CAN_RxFifo0MsgPendingCallback, Uart_Driver_RxCallback via
 *        HAL_UART_RxCpltCallback).
 */
#include "main.h"
#include "stm32f1xx_it.h"

extern CAN_HandleTypeDef  hcan1;
extern UART_HandleTypeDef huart1;

/* ==========================================================================
 * Cortex-M3 core exception handlers
 * ========================================================================*/
void NMI_Handler(void)
{
    while (1) { }
}

void HardFault_Handler(void)
{
    Error_Handler();
}

void MemManage_Handler(void)
{
    Error_Handler();
}

void BusFault_Handler(void)
{
    Error_Handler();
}

void UsageFault_Handler(void)
{
    Error_Handler();
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ==========================================================================
 * Peripheral IRQ handlers
 * ========================================================================*/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
