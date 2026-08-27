/* CMSIS Cortex-M7 Device Peripheral Access Layer System Source File. */

#include <stdint.h>
#include "stm32h7xx.h"

/* Default SystemCoreClock value (will be updated when PLL is configured in PAL) */
uint32_t SystemCoreClock = 64000000UL; /* 64 MHz default HSI */

/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting, vector table location and external memory
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
    /* FPU settings ------------------------------------------------------------*/
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
      SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif

    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= RCC_CR_HSION;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, HSECSSON, CSION, HSI48ON, CSIKERON, PLL1ON, PLL2ON and PLL3ON bits */
    RCC->CR &= 0xEAF6ED7FU;

    /* Reset D1CFGR register */
    RCC->D1CFGR = 0x00000000;

    /* Reset D2CFGR register */
    RCC->D2CFGR = 0x00000000;

    /* Reset D3CFGR register */
    RCC->D3CFGR = 0x00000000;

    /* Reset PLLCKSELR register */
    RCC->PLLCKSELR = 0x02020200;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x01FF0000;

    /* Reset PLL1DIVR register */
    RCC->PLL1DIVR = 0x01010280;

    /* Reset PLL1FRACR register */
    RCC->PLL1FRACR = 0x00000000;

    /* Reset PLL2DIVR register */
    RCC->PLL2DIVR = 0x01010280;

    /* Reset PLL2FRACR register */
    RCC->PLL2FRACR = 0x00000000;

    /* Reset PLL3DIVR register */
    RCC->PLL3DIVR = 0x01010280;

    /* Reset PLL3FRACR register */
    RCC->PLL3FRACR = 0x00000000;

    /* Disable all interrupts */
    RCC->CIER = 0x00000000;

    /* Configure the Vector Table location add offset address ------------------*/
    SCB->VTOR = FLASH_BASE;
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
    /* Will be set precisely by PAL */
}
