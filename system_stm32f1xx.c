/**
 * @file    system_stm32f1xx.c
 * @brief   CMSIS system init. SystemInit() resets the clock-related
 *          registers to a known reset state (actual clock switch to
 *          72MHz happens later in SystemClock_Config(), via HAL_RCC,
 *          once HAL_Init() has run). SystemCoreClockUpdate() recomputes
 *          SystemCoreClock from the live RCC register contents, matching
 *          the standard CMSIS contract other HAL timing code relies on.
 */
#include "stm32f1xx.h"

/* Updated by SystemCoreClockUpdate(); HAL_RCC_ClockConfig() calls it after
 * every reconfiguration, so this stays accurate for HAL_Delay() etc. */
uint32_t SystemCoreClock = HSI_VALUE;

static const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
static const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void)
{
    /* Reset RCC to its power-on state so SystemClock_Config() always starts
     * from a known baseline, regardless of what a bootloader left behind. */
    RCC->CR |= (uint32_t)0x00000001U;      /* HSION */

    RCC->CFGR &= (uint32_t)0xF8FF0000U;    /* reset SW, HPRE, PPRE1, PPRE2, ADCPRE, MCO */

    RCC->CR   &= (uint32_t)0xFEF6FFFFU;    /* reset HSEON, CSSON, PLLON */
    RCC->CR   &= (uint32_t)0xFFFBFFFFU;    /* reset HSEBYP */
    RCC->CFGR &= (uint32_t)0xFF80FFFFU;    /* reset PLLSRC, PLLXTPRE, PLLMUL, USBPRE */
    RCC->CIR   = 0x009F0000U;              /* disable all interrupts, clear pending */

    /* Relocate vector table to the start of flash (default; override with
     * -DVECT_TAB_SRAM if you ever run from RAM for a bootloader). */
    SCB->VTOR = FLASH_BASE;
}

void SystemCoreClockUpdate(void)
{
    uint32_t tmp, pllmull, pllsource;

    tmp = RCC->CFGR & RCC_CFGR_SWS;

    switch (tmp) {
        case 0x04U: /* HSI used as system clock */
            SystemCoreClock = HSI_VALUE;
            break;
        case 0x08U: /* HSE used as system clock */
            SystemCoreClock = HSE_VALUE;
            break;
        case 0x0CU: { /* PLL used as system clock */
            pllmull   = (RCC->CFGR & RCC_CFGR_PLLMULL) >> 18U;
            pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
            pllmull   = pllmull + 2U;

            if (pllsource == 0x00U) {
                SystemCoreClock = (HSI_VALUE >> 1U) * pllmull;
            } else {
                if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != 0U) {
                    SystemCoreClock = (HSE_VALUE >> 1U) * pllmull;
                } else {
                    SystemCoreClock = HSE_VALUE * pllmull;
                }
            }
            break;
        }
        default:
            SystemCoreClock = HSI_VALUE;
            break;
    }

    /* Fold in AHB prescaler */
    tmp = AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE) >> 4U];
    SystemCoreClock >>= tmp;
    (void)APBPrescTable; /* APB1/APB2 clocks are derived on demand by HAL_RCC_Get*ClockFreq() */
}
