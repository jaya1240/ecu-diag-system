/**
 ******************************************************************************
 * @file      startup_stm32f103xb.s
 * @brief     STM32F103xB (medium-density) startup file for GNU (arm-none-eabi-gcc).
 *            Sets the initial SP, sets the vector table entries, branches to
 *            SystemInit(), copies .data from flash to RAM, zeroes .bss, then
 *            calls __libc_init_array() and main().
 ******************************************************************************
 */
  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section */
.word  _sidata
/* start address for the .data section */
.word  _sdata
/* end address for the .data section */
.word  _edata
/* start address for the .bss section */
.word  _sbss
/* end address for the .bss section */
.word  _ebss

  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack        /* set stack pointer */

  /* Copy the data segment initializers from flash to SRAM */
  movs  r1, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r3, =_sidata
  ldr   r3, [r3, r1]
  str   r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr   r0, =_sdata
  ldr   r3, =_edata
  adds  r2, r0, r1
  cmp   r2, r3
  bcc   CopyDataInit
  ldr   r2, =_sbss
  b     LoopFillZerobss

/* Zero fill the bss segment. */
FillZerobss:
  movs  r3, #0
  str   r3, [r2]
  adds  r2, r2, #4

LoopFillZerobss:
  ldr   r3, = _ebss
  cmp   r2, r3
  bcc   FillZerobss

  bl  SystemInit
  bl  __libc_init_array
  bl  main
  bx  lr
.size  Reset_Handler, .-Reset_Handler

/**
 * Default handler for exceptions / IRQs without a specific implementation.
 * Loops forever so a spurious/unhandled interrupt is visible on a debugger
 * rather than silently falling through.
 */
  .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler

/**
 * Minimal vector table for STM32F103xB (medium-density).
 * Only entries this project actually wires up (Reset, SysTick, bxCAN RX0,
 * USART1, and the core fault handlers) point at real symbols; everything
 * else defaults to Default_Handler via the .weak aliases below, so an
 * unused-but-enabled interrupt still lands somewhere sane instead of
 * jumping through a null/garbage vector.
 */
  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler
  .word  WWDG_IRQHandler                 /* 0  */
  .word  PVD_IRQHandler                  /* 1  */
  .word  TAMPER_IRQHandler               /* 2  */
  .word  RTC_IRQHandler                  /* 3  */
  .word  FLASH_IRQHandler                /* 4  */
  .word  RCC_IRQHandler                  /* 5  */
  .word  EXTI0_IRQHandler                /* 6  */
  .word  EXTI1_IRQHandler                /* 7  */
  .word  EXTI2_IRQHandler                /* 8  */
  .word  EXTI3_IRQHandler                /* 9  */
  .word  EXTI4_IRQHandler                /* 10 */
  .word  DMA1_Channel1_IRQHandler        /* 11 */
  .word  DMA1_Channel2_IRQHandler        /* 12 */
  .word  DMA1_Channel3_IRQHandler        /* 13 */
  .word  DMA1_Channel4_IRQHandler        /* 14 */
  .word  DMA1_Channel5_IRQHandler        /* 15 */
  .word  DMA1_Channel6_IRQHandler        /* 16 */
  .word  DMA1_Channel7_IRQHandler        /* 17 */
  .word  ADC1_2_IRQHandler               /* 18 */
  .word  USB_HP_CAN1_TX_IRQHandler       /* 19 */
  .word  USB_LP_CAN1_RX0_IRQHandler      /* 20 - bxCAN RX FIFO0 (used) */
  .word  CAN1_RX1_IRQHandler             /* 21 */
  .word  CAN1_SCE_IRQHandler             /* 22 */
  .word  EXTI9_5_IRQHandler              /* 23 */
  .word  TIM1_BRK_IRQHandler             /* 24 */
  .word  TIM1_UP_IRQHandler              /* 25 */
  .word  TIM1_TRG_COM_IRQHandler         /* 26 */
  .word  TIM1_CC_IRQHandler              /* 27 */
  .word  TIM2_IRQHandler                 /* 28 */
  .word  TIM3_IRQHandler                 /* 29 */
  .word  TIM4_IRQHandler                 /* 30 */
  .word  I2C1_EV_IRQHandler              /* 31 */
  .word  I2C1_ER_IRQHandler              /* 32 */
  .word  I2C2_EV_IRQHandler              /* 33 */
  .word  I2C2_ER_IRQHandler              /* 34 */
  .word  SPI1_IRQHandler                 /* 35 */
  .word  SPI2_IRQHandler                 /* 36 */
  .word  USART1_IRQHandler               /* 37 - console (used) */
  .word  USART2_IRQHandler               /* 38 */
  .word  USART3_IRQHandler               /* 39 */
  .word  EXTI15_10_IRQHandler            /* 40 */
  .word  RTC_Alarm_IRQHandler            /* 41 */
  .word  USBWakeUp_IRQHandler            /* 42 */

/* Every unused handler below is a weak alias for Default_Handler. A
 * project-specific file (like stm32f1xx_it.c) can override any of these
 * by defining a strong symbol with the same name. */
  .weak      NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak      HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak      MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak      BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak      UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak      SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak      DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak      PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak      SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak      WWDG_IRQHandler
  .thumb_set WWDG_IRQHandler,Default_Handler
  .weak      PVD_IRQHandler
  .thumb_set PVD_IRQHandler,Default_Handler
  .weak      TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler,Default_Handler
  .weak      RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler
  .weak      FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler,Default_Handler
  .weak      RCC_IRQHandler
  .thumb_set RCC_IRQHandler,Default_Handler
  .weak      EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler
  .weak      EXTI1_IRQHandler
  .thumb_set EXTI1_IRQHandler,Default_Handler
  .weak      EXTI2_IRQHandler
  .thumb_set EXTI2_IRQHandler,Default_Handler
  .weak      EXTI3_IRQHandler
  .thumb_set EXTI3_IRQHandler,Default_Handler
  .weak      EXTI4_IRQHandler
  .thumb_set EXTI4_IRQHandler,Default_Handler
  .weak      DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler,Default_Handler
  .weak      DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler,Default_Handler
  .weak      DMA1_Channel3_IRQHandler
  .thumb_set DMA1_Channel3_IRQHandler,Default_Handler
  .weak      DMA1_Channel4_IRQHandler
  .thumb_set DMA1_Channel4_IRQHandler,Default_Handler
  .weak      DMA1_Channel5_IRQHandler
  .thumb_set DMA1_Channel5_IRQHandler,Default_Handler
  .weak      DMA1_Channel6_IRQHandler
  .thumb_set DMA1_Channel6_IRQHandler,Default_Handler
  .weak      DMA1_Channel7_IRQHandler
  .thumb_set DMA1_Channel7_IRQHandler,Default_Handler
  .weak      ADC1_2_IRQHandler
  .thumb_set ADC1_2_IRQHandler,Default_Handler
  .weak      USB_HP_CAN1_TX_IRQHandler
  .thumb_set USB_HP_CAN1_TX_IRQHandler,Default_Handler
  .weak      USB_LP_CAN1_RX0_IRQHandler
  .thumb_set USB_LP_CAN1_RX0_IRQHandler,Default_Handler
  .weak      CAN1_RX1_IRQHandler
  .thumb_set CAN1_RX1_IRQHandler,Default_Handler
  .weak      CAN1_SCE_IRQHandler
  .thumb_set CAN1_SCE_IRQHandler,Default_Handler
  .weak      EXTI9_5_IRQHandler
  .thumb_set EXTI9_5_IRQHandler,Default_Handler
  .weak      TIM1_BRK_IRQHandler
  .thumb_set TIM1_BRK_IRQHandler,Default_Handler
  .weak      TIM1_UP_IRQHandler
  .thumb_set TIM1_UP_IRQHandler,Default_Handler
  .weak      TIM1_TRG_COM_IRQHandler
  .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler
  .weak      TIM1_CC_IRQHandler
  .thumb_set TIM1_CC_IRQHandler,Default_Handler
  .weak      TIM2_IRQHandler
  .thumb_set TIM2_IRQHandler,Default_Handler
  .weak      TIM3_IRQHandler
  .thumb_set TIM3_IRQHandler,Default_Handler
  .weak      TIM4_IRQHandler
  .thumb_set TIM4_IRQHandler,Default_Handler
  .weak      I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler
  .weak      I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler
  .weak      I2C2_EV_IRQHandler
  .thumb_set I2C2_EV_IRQHandler,Default_Handler
  .weak      I2C2_ER_IRQHandler
  .thumb_set I2C2_ER_IRQHandler,Default_Handler
  .weak      SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler
  .weak      SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler
  .weak      USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler
  .weak      USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler
  .weak      USART3_IRQHandler
  .thumb_set USART3_IRQHandler,Default_Handler
  .weak      EXTI15_10_IRQHandler
  .thumb_set EXTI15_10_IRQHandler,Default_Handler
  .weak      RTC_Alarm_IRQHandler
  .thumb_set RTC_Alarm_IRQHandler,Default_Handler
  .weak      USBWakeUp_IRQHandler
  .thumb_set USBWakeUp_IRQHandler,Default_Handler
