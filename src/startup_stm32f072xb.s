/**
  ******************************************************************************
  * @file      startup_stm32f072xb.s
  * @brief     STM32F072xB vector table and startup code for GNU toolchain
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m0
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

/* Linker symbols */
.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   r0, =_estack
  mov   sp, r0

  /* Copy the data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit

  /* Zero fill the bss segment */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

  /* Call static constructors */
  bl __libc_init_array
  /* Call the application entry point */
  bl main

LoopForever:
  b LoopForever

.size Reset_Handler, .-Reset_Handler

  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  0
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler

  /* External Interrupts */
  .word  WWDG_IRQHandler
  .word  PVD_VDDIO2_IRQHandler
  .word  RTC_IRQHandler
  .word  FLASH_IRQHandler
  .word  RCC_CRS_IRQHandler
  .word  EXTI0_1_IRQHandler
  .word  EXTI2_3_IRQHandler
  .word  EXTI4_15_IRQHandler
  .word  TSC_IRQHandler
  .word  DMA1_Ch1_IRQHandler
  .word  DMA1_Ch2_3_DMA2_Ch1_2_IRQHandler
  .word  DMA1_Ch4_5_6_7_DMA2_Ch3_4_5_IRQHandler
  .word  ADC1_COMP_IRQHandler
  .word  TIM1_BRK_UP_TRG_COM_IRQHandler
  .word  TIM1_CC_IRQHandler
  .word  TIM2_IRQHandler
  .word  TIM3_IRQHandler
  .word  TIM6_DAC_IRQHandler
  .word  TIM7_IRQHandler
  .word  TIM14_IRQHandler
  .word  TIM15_IRQHandler
  .word  TIM16_IRQHandler
  .word  TIM17_IRQHandler
  .word  I2C1_IRQHandler
  .word  I2C2_IRQHandler
  .word  SPI1_IRQHandler
  .word  SPI2_IRQHandler
  .word  USART1_IRQHandler
  .word  USART2_IRQHandler
  .word  USART3_4_IRQHandler
  .word  CEC_CAN_IRQHandler
  .word  USB_IRQHandler

  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .macro def_irq_handler handler_name
    .weak \handler_name
    .thumb_set \handler_name,Default_Handler
  .endm

  def_irq_handler WWDG_IRQHandler
  def_irq_handler PVD_VDDIO2_IRQHandler
  def_irq_handler RTC_IRQHandler
  def_irq_handler FLASH_IRQHandler
  def_irq_handler RCC_CRS_IRQHandler
  def_irq_handler EXTI0_1_IRQHandler
  def_irq_handler EXTI2_3_IRQHandler
  def_irq_handler EXTI4_15_IRQHandler
  def_irq_handler TSC_IRQHandler
  def_irq_handler DMA1_Ch1_IRQHandler
  def_irq_handler DMA1_Ch2_3_DMA2_Ch1_2_IRQHandler
  def_irq_handler DMA1_Ch4_5_6_7_DMA2_Ch3_4_5_IRQHandler
  def_irq_handler ADC1_COMP_IRQHandler
  def_irq_handler TIM1_BRK_UP_TRG_COM_IRQHandler
  def_irq_handler TIM1_CC_IRQHandler
  def_irq_handler TIM2_IRQHandler
  def_irq_handler TIM3_IRQHandler
  def_irq_handler TIM6_DAC_IRQHandler
  def_irq_handler TIM7_IRQHandler
  def_irq_handler TIM14_IRQHandler
  def_irq_handler TIM15_IRQHandler
  def_irq_handler TIM16_IRQHandler
  def_irq_handler TIM17_IRQHandler
  def_irq_handler I2C1_IRQHandler
  def_irq_handler I2C2_IRQHandler
  def_irq_handler SPI1_IRQHandler
  def_irq_handler SPI2_IRQHandler
  def_irq_handler USART1_IRQHandler
  def_irq_handler USART2_IRQHandler
  def_irq_handler USART3_4_IRQHandler
  def_irq_handler CEC_CAN_IRQHandler
  def_irq_handler USB_IRQHandler
