/**
 * @file    main.h
 * @brief   Application entry point declarations and global config.
 */
#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"   /* Swap for stm32f4xx_hal.h etc. per target */

/* ---- Scheduler tick ---------------------------------------------------- */
#define SYSTEM_TICK_MS          1U     /* SysTick period                    */
#define FAULT_SIM_PERIOD_MS     2000U  /* how often fault simulator runs    */
#define TESTER_PRESENT_TIMEOUT_MS 5000U /* extended session timeout         */

/* ---- Pin mapping (STM32F103C8T6 "Blue Pill" reference) ----------------- */
#define CAN_RX_GPIO_Port        GPIOA
#define CAN_RX_Pin               GPIO_PIN_11
#define CAN_TX_GPIO_Port        GPIOA
#define CAN_TX_Pin               GPIO_PIN_12

#define STATUS_LED_GPIO_Port    GPIOC
#define STATUS_LED_Pin           GPIO_PIN_13

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
