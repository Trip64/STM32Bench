/* Platform Abstraction Layer implementation for STM32H7 series */

#include "pal.h"
#include "stm32h7xx.h"
#include "bsp_nucleo_h723zg.h"

static volatile uint32_t s_systick_ms = 0;
static uint32_t s_core_clock_hz = 64000000UL;

/* SysTick Interrupt Handler */
void SysTick_Handler(void)
{
    s_systick_ms++;
}

/* Enable DWT Cycle Counter with proper unlock sequence */
static void PAL_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    #ifdef DWT_LAR_KEY
    DWT->LAR = 0xC5ACCE55;
    #else
    *((volatile uint32_t*)0xE0001FB0) = 0xC5ACCE55; /* Unlock LAR if present */
    #endif
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* Configure Clocks for STM32H723 (targeting up to 550 MHz) */
static void PAL_Clock_Init(void)
{
    uint32_t to = 100000;

    /* 1. Configure VOS0 (Highest performance mode for 550 MHz in H723) with timeout */
    PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS) | (3U << 14); /* VOS0 = 3 in H723 */
    to = 100000;
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY) && --to) {}

    /* 2. Flash Memory Latency: 7 wait states for 550 MHz */
    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_WRHIGHFREQ_1;
    to = 100000;
    while (((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_7WS) && --to) {}

    /* 3. Configure PLL1:
     * Try HSE (25 MHz). Nucleo-144 has 25 MHz from ST-Link MCO or crystal.
     * On Nucleo, ST-Link clock can be bypass or crystal. Enable HSE with bypass bit.
     */
    RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
    to = 100000;
    while (!(RCC->CR & RCC_CR_HSERDY) && --to) {}

    bool use_hse = (to > 0);

    /* Ensure PLL1 is OFF before configuring */
    RCC->CR &= ~RCC_CR_PLL1ON;
    to = 100000;
    while ((RCC->CR & RCC_CR_PLL1RDY) && --to) {}

    if (use_hse) {
        /* HSE = 25 MHz.
         * DIVM1 = 5 -> Ref clock = 5 MHz
         * DIVN1 = 110 -> VCO = 550 MHz
         * DIVP1 = 1 -> SysClk = 550 MHz
         */
        RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_PLLSRC) | RCC_PLLCKSELR_PLLSRC_HSE;
        RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_DIVM1) | (5U << 4);

        RCC->PLL1DIVR = ((1U - 1U) << 9) |   /* DIVP1 = 1 */
                        ((4U - 1U) << 16) |  /* DIVQ1 = 4 */
                        ((2U - 1U) << 24) |  /* DIVR1 = 2 */
                        (110U - 1U);         /* DIVN1 = 110 */

        RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2; /* 4-8 MHz input range */
        RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL; /* Wide VCO range: 192 - 960 MHz */
        RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_DIVQ1EN;

        s_core_clock_hz = 550000000UL;
    } else {
        /* HSI = 64 MHz fallback
         * DIVM1 = 8 -> Ref = 8 MHz
         * DIVN1 = 68 -> VCO = 544 MHz
         * DIVP1 = 1 -> SysClk = 544 MHz
         */
        RCC->CR |= RCC_CR_HSION;
        to = 100000;
        while (!(RCC->CR & RCC_CR_HSIRDY) && --to) {}

        RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_PLLSRC) | RCC_PLLCKSELR_PLLSRC_HSI;
        RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_DIVM1) | (8U << 4);

        RCC->PLL1DIVR = ((1U - 1U) << 9) |   /* DIVP1 = 1 */
                        ((4U - 1U) << 16) |  /* DIVQ1 = 4 */
                        ((2U - 1U) << 24) |  /* DIVR1 = 2 */
                        (68U - 1U);          /* DIVN1 = 68 */

        RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2;
        RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL;
        RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_DIVQ1EN;

        s_core_clock_hz = 544000000UL;
    }

    /* Enable PLL1 with timeout */
    RCC->CR |= RCC_CR_PLL1ON;
    to = 200000;
    while (!(RCC->CR & RCC_CR_PLL1RDY) && --to) {}

    if (to > 0) {
        /* Configure bus clock prescalers:
         * D1CPRE = /1 (550 MHz CPU)
         * HPRE = /2 (275 MHz AXI, AHB1/2/3/4)
         * D1PPRE = /2 (137.5 MHz APB3)
         * D2PPRE1 = /2 (137.5 MHz APB1)
         * D2PPRE2 = /2 (137.5 MHz APB2)
         * D3PPRE = /2 (137.5 MHz APB4)
         */
        RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_D1CPRE) | RCC_D1CFGR_D1CPRE_DIV1;
        RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_HPRE)   | RCC_D1CFGR_HPRE_DIV2;
        RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_D1PPRE) | RCC_D1CFGR_D1PPRE_DIV2;
        RCC->D2CFGR = (RCC->D2CFGR & ~RCC_D2CFGR_D2PPRE1)| RCC_D2CFGR_D2PPRE1_DIV2;
        RCC->D2CFGR = (RCC->D2CFGR & ~RCC_D2CFGR_D2PPRE2)| RCC_D2CFGR_D2PPRE2_DIV2;
        RCC->D3CFGR = (RCC->D3CFGR & ~RCC_D3CFGR_D3PPRE) | RCC_D3CFGR_D3PPRE_DIV2;

        /* Switch System Clock to PLL1P with timeout */
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
        to = 100000;
        while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) && --to) {}
    } else {
        /* If PLL failed, stay on HSI 64 MHz */
        s_core_clock_hz = 64000000UL;
    }

    SystemCoreClock = s_core_clock_hz;

    /* Re-init SysTick with accurate core frequency */
    SysTick_Config(s_core_clock_hz / 1000UL);
}

void PAL_Init(void)
{
    /* Initialize board LEDs and Buttons */
    BSP_Init();

    /* Initialize system clocks and PLL first */
    PAL_Clock_Init();

    /* Initialize UART3 (115200 baud) on verified 64 MHz HSI */
    PAL_UART_Init(115200);

    /* Enable I-Cache and D-Cache */
    PAL_EnableICache();
    PAL_EnableDCache();

    /* Initialize DWT cycle counter */
    PAL_DWT_Init();
}

uint32_t PAL_GetCoreClockHz(void)
{
    return s_core_clock_hz;
}

uint32_t PAL_GetCycleCount(void)
{
    return DWT->CYCCNT;
}

uint32_t PAL_GetMillis(void)
{
    return s_systick_ms;
}

void PAL_DelayMs(uint32_t ms)
{
    uint32_t start = s_systick_ms;
    while ((s_systick_ms - start) < ms) {
        __WFI();
    }
}

/* Cache Control */
void PAL_EnableICache(void)
{
    SCB_EnableICache();
}

void PAL_DisableICache(void)
{
    SCB_DisableICache();
}

void PAL_EnableDCache(void)
{
    SCB_EnableDCache();
}

void PAL_DisableDCache(void)
{
    SCB_DisableDCache();
}

void PAL_CleanDCache(void)
{
    SCB_CleanDCache();
}

void PAL_CleanInvalidateDCache(void)
{
    SCB_CleanInvalidateDCache();
}

/* Hardware Feature Detection */
bool PAL_HasFPU(void)
{
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    return true;
    #else
    return false;
    #endif
}

bool PAL_HasDPFPU(void)
{
    #if defined(__ARM_FP) && (__ARM_FP & 0x08)
    return true; /* Double precision supported */
    #else
    return false;
    #endif
}

bool PAL_HasDSP(void)
{
    #if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
    return true;
    #else
    return false;
    #endif
}

bool PAL_HasCORDIC(void)
{
    #if defined(RCC_AHB2ENR_CORDICEN) || defined(RCC_AHB1ENR_CORDICEN)
    return true;
    #else
    return false;
    #endif
}

bool PAL_HasFMAC(void)
{
    #if defined(RCC_AHB2ENR_FMACEN) || defined(RCC_AHB1ENR_FMACEN)
    return true;
    #else
    return false;
    #endif
}

bool PAL_HasICache(void)
{
    return (SCB->CCR & SCB_CCR_IC_Msk) != 0;
}

bool PAL_HasDCache(void)
{
    return (SCB->CCR & SCB_CCR_DC_Msk) != 0;
}

bool PAL_HasDMA2D(void)
{
    #if defined(RCC_AHB3ENR_DMA2DEN) || defined(DMA2D)
    return true;
    #else
    return false;
    #endif
}

bool PAL_HasRNG(void)
{
    #if defined(RCC_AHB2ENR_RNGEN) || defined(RNG)
    return true;
    #else
    return false;
    #endif
}

/* Chip Identification */
const char* PAL_GetChipName(void)
{
    return "STM32H723ZGT6";
}

const char* PAL_GetCoreName(void)
{
    return "Arm Cortex-M7 (r1p1)";
}

uint32_t PAL_GetFlashSizeKB(void)
{
    /* Flash size register at 0x1FF1E880 */
    uint16_t size_kb = *((volatile uint16_t*)0x1FF1E880);
    return (size_kb == 0 || size_kb == 0xFFFF) ? 1024 : size_kb;
}

uint32_t PAL_GetRAMSizeKB(void)
{
    return 564; /* 64K ITCM + 128K DTCM + 320K AXI + 32K D2 + 16K D3 */
}

uint32_t PAL_GetCPUID(void)
{
    return SCB->CPUID;
}

uint16_t PAL_GetDeviceID(void)
{
    return (uint16_t)(DBGMCU->IDCODE & 0xFFF);
}

uint16_t PAL_GetRevisionID(void)
{
    return (uint16_t)((DBGMCU->IDCODE >> 16) & 0xFFFF);
}

void PAL_GetUID(uint32_t uid[3])
{
    /* STM32 unique device ID at 0x1FF1E800 */
    uid[0] = *((volatile uint32_t*)0x1FF1E800);
    uid[1] = *((volatile uint32_t*)0x1FF1E804);
    uid[2] = *((volatile uint32_t*)0x1FF1E808);
}

/* UART3 configuration for ST-LINK VCP (PD8=TX, PD9=RX) */
void PAL_UART_Init(uint32_t baudrate)
{
    /* 1. Ensure HSI is enabled and undivided (64 MHz) */
    RCC->CR |= RCC_CR_HSION;
    uint32_t to = 100000;
    while (!(RCC->CR & RCC_CR_HSIRDY) && --to) {}

    /* Clear HSI divider to 1 (64 MHz) and wait for flag */
    RCC->CR &= ~RCC_CR_HSIDIV;
    to = 100000;
    while (!(RCC->CR & RCC_CR_HSIDIVF) && --to) {}

    /* 2. Select HSI (64 MHz) as USART234578 kernel clock source in RCC_D2CCIP2R
     * Bits 2:0 = 011b (3) selects hsi_ker_ck
     */
    RCC->D2CCIP2R = (RCC->D2CCIP2R & ~RCC_D2CCIP2R_USART28SEL) | (3U << 0);

    /* 3. Enable GPIOD and USART3 clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    __DSB();

    /* 4. PD8 (TX) -> AF7, PD9 (RX) -> AF7 */
    GPIOD->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOD->MODER |=  ((2U << (8 * 2)) | (2U << (9 * 2))); /* AF mode */

    GPIOD->AFR[1] &= ~((0xFU << ((8 - 8) * 4)) | (0xFU << ((9 - 8) * 4)));
    GPIOD->AFR[1] |=  ((7U << ((8 - 8) * 4)) | (7U << ((9 - 8) * 4))); /* AF7 */

    GPIOD->OSPEEDR |= ((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOD->PUPDR   &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOD->PUPDR   |= ((1U << (8 * 2)) | (1U << (9 * 2))); /* Pull-up */

    /* 5. Calculate BRR from dedicated 64 MHz HSI clock */
    uint32_t ker_clock = 64000000UL;
    USART3->CR1 = 0; /* Disable USART */
    USART3->BRR = (ker_clock + (baudrate / 2)) / baudrate;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void PAL_UART_WriteChar(char c)
{
    uint32_t to = 100000;
    while (!(USART3->ISR & USART_ISR_TXE_TXFNF) && --to) {}
    if (to > 0) {
        USART3->TDR = (uint8_t)c;
    }
}

void PAL_UART_WriteString(const char* str)
{
    if (!str) return;
    while (*str) {
        if (*str == '\n') PAL_UART_WriteChar('\r');
        PAL_UART_WriteChar(*str++);
    }
}

void PAL_UART_WriteBytes(const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        PAL_UART_WriteChar((char)data[i]);
    }
}

bool PAL_UART_HasChar(void)
{
    if (USART3->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_PE | USART_ISR_NE))
        USART3->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF | USART_ICR_NECF;
    return (USART3->ISR & USART_ISR_RXNE_RXFNE) != 0;
}

char PAL_UART_ReadChar(void)
{
    return (char)(USART3->RDR & 0xFF);
}

/* LED & Button wrappers */
void PAL_LED_Init(void) { BSP_LED_Init(); }
void PAL_LED_Set(int led_index, bool state)
{
    if (state) BSP_LED_On(led_index);
    else BSP_LED_Off(led_index);
}
void PAL_LED_Toggle(int led_index) { BSP_LED_Toggle(led_index); }
bool PAL_Button_Read(void) { return BSP_Button_IsPressed(); }
