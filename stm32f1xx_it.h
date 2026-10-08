/**
 * @file stm32f1xx_it.h
 * @brief Cortex-M3 core exception and peripheral IRQ handler declarations.
 */
#ifndef STM32F1xx_IT_H
#define STM32F1xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Cortex-M3 core exceptions */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* Peripheral IRQs used by this project */
void USB_LP_CAN1_RX0_IRQHandler(void); /* bxCAN RX FIFO0 (shared with USB on medium-density F103) */
void USART1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F1xx_IT_H */
