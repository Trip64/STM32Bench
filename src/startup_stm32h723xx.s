/**
  ******************************************************************************
  * @file      startup_stm32h723xx.s
  * @brief     STM32H723xx vector table and startup code for GCC toolchain
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m7
  .fpu fpv5-d16
  .thumb

.global g_pfnVectors
.global Default_Handler
.global Reset_Handler

/* Linker script symbols */
.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss
.word _siitcm
.word _sitcm
.word _eitcm

  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack      /* Set stack pointer */

/* Call the clock and system configuration function */
  bl  SystemInit

/* Copy the data segment initializers from flash to SRAM */
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit

/* Copy ITCM text from flash to ITCMRAM */
  ldr  r0, =_sitcm
  ldr  r1, =_eitcm
  ldr  r2, =_siitcm
  cmp  r0, r1
  beq  ZeroBssInit

CopyItcmLoop:
  cmp  r0, r1
  bcs  ZeroBssInit
  ldr  r3, [r2], #4
  str  r3, [r0], #4
  b    CopyItcmLoop

/* Zero fill the bss segment */
ZeroBssInit:
  ldr  r2, =_sbss
  b  LoopFillZerobss

FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
  bcc  FillZerobss

/* Call static constructors */
  bl __libc_init_array
/* Call the application's entry point */
  bl main
  bx lr
.size Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.
 */
  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/******************************************************************************
* Vectors table
******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler

  /* External Interrupts */
  .word WWDG_IRQHandler
  .word PVD_AVD_IRQHandler
  .word TAMP_STAMP_IRQHandler
  .word RTC_WKUP_IRQHandler
  .word FLASH_IRQHandler
  .word RCC_IRQHandler
  .word EXTI0_IRQHandler
  .word EXTI1_IRQHandler
  .word EXTI2_IRQHandler
  .word EXTI3_IRQHandler
  .word EXTI4_IRQHandler
  .word DMA1_Stream0_IRQHandler
  .word DMA1_Stream1_IRQHandler
  .word DMA1_Stream2_IRQHandler
  .word DMA1_Stream3_IRQHandler
  .word DMA1_Stream4_IRQHandler
  .word DMA1_Stream5_IRQHandler
  .word DMA1_Stream6_IRQHandler
  .word ADC_IRQHandler
  .word FDCAN1_IT0_IRQHandler
  .word FDCAN2_IT0_IRQHandler
  .word FDCAN1_IT1_IRQHandler
  .word FDCAN2_IT1_IRQHandler
  .word EXTI9_5_IRQHandler
  .word TIM1_BRK_IRQHandler
  .word TIM1_UP_IRQHandler
  .word TIM1_TRG_COM_IRQHandler
  .word TIM1_CC_IRQHandler
  .word TIM2_IRQHandler
  .word TIM3_IRQHandler
  .word TIM4_IRQHandler
  .word I2C1_EV_IRQHandler
  .word I2C1_ER_IRQHandler
  .word I2C2_EV_IRQHandler
  .word I2C2_ER_IRQHandler
  .word SPI1_IRQHandler
  .word SPI2_IRQHandler
  .word USART1_IRQHandler
  .word USART2_IRQHandler
  .word USART3_IRQHandler
  .word EXTI15_10_IRQHandler
  .word RTC_Alarm_IRQHandler
  .word 0
  .word TIM8_BRK_TIM12_IRQHandler
  .word TIM8_UP_TIM13_IRQHandler
  .word TIM8_TRG_COM_TIM14_IRQHandler
  .word TIM8_CC_IRQHandler
  .word DMA1_Stream7_IRQHandler
  .word FMC_IRQHandler
  .word SDMMC1_IRQHandler
  .word TIM5_IRQHandler
  .word SPI3_IRQHandler
  .word UART4_IRQHandler
  .word UART5_IRQHandler
  .word TIM6_DAC_IRQHandler
  .word TIM7_IRQHandler
  .word DMA2_Stream0_IRQHandler
  .word DMA2_Stream1_IRQHandler
  .word DMA2_Stream2_IRQHandler
  .word DMA2_Stream3_IRQHandler
  .word DMA2_Stream4_IRQHandler
  .word ETH_IRQHandler
  .word ETH_WKUP_IRQHandler
  .word FDCAN_CAL_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word DMA2_Stream5_IRQHandler
  .word DMA2_Stream6_IRQHandler
  .word DMA2_Stream7_IRQHandler
  .word USART6_IRQHandler
  .word I2C3_EV_IRQHandler
  .word I2C3_ER_IRQHandler
  .word OTG_HS_EP1_OUT_IRQHandler
  .word OTG_HS_EP1_IN_IRQHandler
  .word OTG_HS_WKUP_IRQHandler
  .word OTG_HS_IRQHandler
  .word DCMI_PSSI_IRQHandler
  .word CRYP_IRQHandler
  .word HASH_RNG_IRQHandler
  .word FPU_IRQHandler
  .word UART7_IRQHandler
  .word UART8_IRQHandler
  .word SPI4_IRQHandler
  .word SPI5_IRQHandler
  .word SPI6_IRQHandler
  .word SAI1_IRQHandler
  .word LTDC_IRQHandler
  .word LTDC_ER_IRQHandler
  .word DMA2D_IRQHandler
  .word 0
  .word OCTOSPI1_IRQHandler
  .word LPTIM1_IRQHandler
  .word CEC_IRQHandler
  .word I2C4_EV_IRQHandler
  .word I2C4_ER_IRQHandler
  .word SPDIF_RX_IRQHandler
  .word OTG_FS_EP1_OUT_IRQHandler
  .word OTG_FS_EP1_IN_IRQHandler
  .word OTG_FS_WKUP_IRQHandler
  .word OTG_FS_IRQHandler
  .word DMAMUX1_OVR_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word CORDIC_IRQHandler
  .word FMAC_IRQHandler

/*******************************************************************************
* Provide weak aliases for each Exception handler to the Default_Handler.
*******************************************************************************/
  .macro def_irq_handler handler_name
  .weak \handler_name
  .thumb_set \handler_name,Default_Handler
  .endm

  def_irq_handler NMI_Handler
  def_irq_handler HardFault_Handler
  def_irq_handler MemManage_Handler
  def_irq_handler BusFault_Handler
  def_irq_handler UsageFault_Handler
  def_irq_handler SVC_Handler
  def_irq_handler DebugMon_Handler
  def_irq_handler PendSV_Handler
  def_irq_handler SysTick_Handler

  def_irq_handler WWDG_IRQHandler
  def_irq_handler PVD_AVD_IRQHandler
  def_irq_handler TAMP_STAMP_IRQHandler
  def_irq_handler RTC_WKUP_IRQHandler
  def_irq_handler FLASH_IRQHandler
  def_irq_handler RCC_IRQHandler
  def_irq_handler EXTI0_IRQHandler
  def_irq_handler EXTI1_IRQHandler
  def_irq_handler EXTI2_IRQHandler
  def_irq_handler EXTI3_IRQHandler
  def_irq_handler EXTI4_IRQHandler
  def_irq_handler DMA1_Stream0_IRQHandler
  def_irq_handler DMA1_Stream1_IRQHandler
  def_irq_handler DMA1_Stream2_IRQHandler
  def_irq_handler DMA1_Stream3_IRQHandler
  def_irq_handler DMA1_Stream4_IRQHandler
  def_irq_handler DMA1_Stream5_IRQHandler
  def_irq_handler DMA1_Stream6_IRQHandler
  def_irq_handler ADC_IRQHandler
  def_irq_handler FDCAN1_IT0_IRQHandler
  def_irq_handler FDCAN2_IT0_IRQHandler
  def_irq_handler FDCAN1_IT1_IRQHandler
  def_irq_handler FDCAN2_IT1_IRQHandler
  def_irq_handler EXTI9_5_IRQHandler
  def_irq_handler TIM1_BRK_IRQHandler
  def_irq_handler TIM1_UP_IRQHandler
  def_irq_handler TIM1_TRG_COM_IRQHandler
  def_irq_handler TIM1_CC_IRQHandler
  def_irq_handler TIM2_IRQHandler
  def_irq_handler TIM3_IRQHandler
  def_irq_handler TIM4_IRQHandler
  def_irq_handler I2C1_EV_IRQHandler
  def_irq_handler I2C1_ER_IRQHandler
  def_irq_handler I2C2_EV_IRQHandler
  def_irq_handler I2C2_ER_IRQHandler
  def_irq_handler SPI1_IRQHandler
  def_irq_handler SPI2_IRQHandler
  def_irq_handler USART1_IRQHandler
  def_irq_handler USART2_IRQHandler
  def_irq_handler USART3_IRQHandler
  def_irq_handler EXTI15_10_IRQHandler
  def_irq_handler RTC_Alarm_IRQHandler
  def_irq_handler TIM8_BRK_TIM12_IRQHandler
  def_irq_handler TIM8_UP_TIM13_IRQHandler
  def_irq_handler TIM8_TRG_COM_TIM14_IRQHandler
  def_irq_handler TIM8_CC_IRQHandler
  def_irq_handler DMA1_Stream7_IRQHandler
  def_irq_handler FMC_IRQHandler
  def_irq_handler SDMMC1_IRQHandler
  def_irq_handler TIM5_IRQHandler
  def_irq_handler SPI3_IRQHandler
  def_irq_handler UART4_IRQHandler
  def_irq_handler UART5_IRQHandler
  def_irq_handler TIM6_DAC_IRQHandler
  def_irq_handler TIM7_IRQHandler
  def_irq_handler DMA2_Stream0_IRQHandler
  def_irq_handler DMA2_Stream1_IRQHandler
  def_irq_handler DMA2_Stream2_IRQHandler
  def_irq_handler DMA2_Stream3_IRQHandler
  def_irq_handler DMA2_Stream4_IRQHandler
  def_irq_handler ETH_IRQHandler
  def_irq_handler ETH_WKUP_IRQHandler
  def_irq_handler FDCAN_CAL_IRQHandler
  def_irq_handler DMA2_Stream5_IRQHandler
  def_irq_handler DMA2_Stream6_IRQHandler
  def_irq_handler DMA2_Stream7_IRQHandler
  def_irq_handler USART6_IRQHandler
  def_irq_handler I2C3_EV_IRQHandler
  def_irq_handler I2C3_ER_IRQHandler
  def_irq_handler OTG_HS_EP1_OUT_IRQHandler
  def_irq_handler OTG_HS_EP1_IN_IRQHandler
  def_irq_handler OTG_HS_WKUP_IRQHandler
  def_irq_handler OTG_HS_IRQHandler
  def_irq_handler DCMI_PSSI_IRQHandler
  def_irq_handler CRYP_IRQHandler
  def_irq_handler HASH_RNG_IRQHandler
  def_irq_handler FPU_IRQHandler
  def_irq_handler UART7_IRQHandler
  def_irq_handler UART8_IRQHandler
  def_irq_handler SPI4_IRQHandler
  def_irq_handler SPI5_IRQHandler
  def_irq_handler SPI6_IRQHandler
  def_irq_handler SAI1_IRQHandler
  def_irq_handler LTDC_IRQHandler
  def_irq_handler LTDC_ER_IRQHandler
  def_irq_handler DMA2D_IRQHandler
  def_irq_handler OCTOSPI1_IRQHandler
  def_irq_handler LPTIM1_IRQHandler
  def_irq_handler CEC_IRQHandler
  def_irq_handler I2C4_EV_IRQHandler
  def_irq_handler I2C4_ER_IRQHandler
  def_irq_handler SPDIF_RX_IRQHandler
  def_irq_handler OTG_FS_EP1_OUT_IRQHandler
  def_irq_handler OTG_FS_EP1_IN_IRQHandler
  def_irq_handler OTG_FS_WKUP_IRQHandler
  def_irq_handler OTG_FS_IRQHandler
  def_irq_handler DMAMUX1_OVR_IRQHandler
  def_irq_handler CORDIC_IRQHandler
  def_irq_handler FMAC_IRQHandler
