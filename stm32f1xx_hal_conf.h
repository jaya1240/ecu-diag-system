/**
 * @file    stm32f1xx_hal_conf.h
 * @brief   HAL configuration: which HAL modules are compiled in, and the
 *          board's oscillator/voltage constants. Trimmed to only what this
 *          project uses (RCC, GPIO, CORTEX, CAN, UART, FLASH, PWR, DMA).
 */
#ifndef STM32F1xx_HAL_CONF_H
#define STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Module enables ------------------------------------------------------*/
#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_CAN_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* ---- Oscillator values (Blue Pill: 8MHz HSE crystal) ---------------------*/
#if !defined(HSE_VALUE)
#define HSE_VALUE               8000000U
#endif
#define HSE_STARTUP_TIMEOUT      100U

#if !defined(HSI_VALUE)
#define HSI_VALUE                8000000U
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE                32768U
#endif
#define LSE_STARTUP_TIMEOUT      5000U

#if !defined(LSI_VALUE)
#define LSI_VALUE                40000U
#endif

#define VDD_VALUE                3300U
#define TICK_INT_PRIORITY        0U
#define USE_RTOS                 0U
#define PREFETCH_ENABLE          1U

/* ---- Assert (no-op in release) -------------------------------------------*/
#define assert_param(expr) ((void)0U)

#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_pwr.h"
#include "stm32f1xx_hal_can.h"
#include "stm32f1xx_hal_uart.h"
#include "stm32f1xx_hal_exti.h"

#ifdef __cplusplus
}
#endif

#endif /* STM32F1xx_HAL_CONF_H */
