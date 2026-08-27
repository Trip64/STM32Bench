/* Platform Abstraction Layer implementation for STM32F0 (Cortex-M0) */

#include "pal.h"
#include "bsp.h"
#include "stm32f072xb.h"
#include <string.h>

static uint32_t s_sysclk_hz = 48000000;

void PAL_Init(void)
{
    /* 1. Universal Clock Configuration: 48 MHz via PLL from 8 MHz HSI */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) {}

    /* Flash latency: 1 wait state for 48 MHz */
    FLASH->ACR = FLASH_ACR_PRFTBE | (1U << FLASH_ACR_LATENCY_Pos);

    /* PLL config: HSI/2 (4 MHz) * 12 = 48 MHz */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= (RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLMUL12);

    /* Turn on PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}

    /* Select PLL as system clock */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    s_sysclk_hz = 48000000;

    /* 2. Configure TIM2 as 32-bit hardware cycle counter at full CPU clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();

    TIM2->CR1 = 0;
    TIM2->PSC = 0;            /* 1 tick = 1 CPU clock cycle */
    TIM2->ARR = 0xFFFFFFFF;    /* 32-bit free-running counter */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN; /* Start counter */

    /* 3. Initialize Board LEDs & Button */
    BSP_Init();

    /* 4. Initialize UART (USART2 @ 115200 for ST-Link VCP on Nucleo-F072RB) */
    PAL_UART_Init(115200);
}

/* Clock & Timing */
uint32_t PAL_GetCoreClockHz(void)
{
    return s_sysclk_hz;
}

uint32_t PAL_GetCycleCount(void)
{
    return TIM2->CNT;
}

uint32_t PAL_GetMillis(void)
{
    return TIM2->CNT / (s_sysclk_hz / 1000);
}

void PAL_DelayMs(uint32_t ms)
{
    uint32_t cycles_per_ms = s_sysclk_hz / 1000;
    for (uint32_t i = 0; i < ms; i++) {
        uint32_t start = TIM2->CNT;
        while ((TIM2->CNT - start) < cycles_per_ms) {}
    }
}

/* Low-level UART for ST-Link VCP on Nucleo-F072RB */
void PAL_UART_Init(uint32_t baudrate)
{
    /* Enable GPIOA and USART2 clocks */
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    __DSB();

    /* Configure PA2 (TX) and PA3 (RX) as Alternate Function AF1 */
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2))); /* AF mode */

    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((1U << (2 * 4))   | (1U << (3 * 4)));   /* AF1 = USART2 */

    GPIOA->OSPEEDR |= ((3U << (2 * 2)) | (3U << (3 * 2))); /* High speed */

    /* USART2 Configuration: 8N1 @ baud */
    USART2->CR1 = 0;
    USART2->BRR = s_sysclk_hz / baudrate;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void PAL_UART_WriteChar(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE)) {}
    USART2->TDR = (uint8_t)c;
}

void PAL_UART_WriteString(const char *str)
{
    if (!str) return;
    while (*str) {
        if (*str == '\n') PAL_UART_WriteChar('\r');
        PAL_UART_WriteChar(*str++);
    }
}

void PAL_UART_WriteBytes(const uint8_t *data, size_t len)
{
    if (!data) return;
    for (size_t i = 0; i < len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE)) {}
        USART2->TDR = data[i];
    }
}

bool PAL_UART_HasChar(void)
{
    if (USART2->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_PE | USART_ISR_NE))
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF | USART_ICR_NCF;
    return (USART2->ISR & USART_ISR_RXNE) != 0;
}

char PAL_UART_ReadChar(void)
{
    return (char)(USART2->RDR & 0xFF);
}

/* Cortex-M0 / STM32F0 Hardware Feature Flags */
bool PAL_HasFPU(void)    { return false; } /* Software emulation */
bool PAL_HasDPFPU(void)  { return false; }
bool PAL_HasDSP(void)    { return false; }
bool PAL_HasCORDIC(void) { return false; }
bool PAL_HasFMAC(void)   { return false; }
bool PAL_HasICache(void) { return false; }
bool PAL_HasDCache(void) { return false; }
bool PAL_HasDMA2D(void)  { return false; }
bool PAL_HasRNG(void)    { return false; }

/* Cache stubs (no-op on Cortex-M0) */
void PAL_EnableICache(void) {}
void PAL_DisableICache(void) {}
void PAL_EnableDCache(void) {}
void PAL_DisableDCache(void) {}
void PAL_CleanDCache(void) {}
void PAL_CleanInvalidateDCache(void) {}

/* Chip Info */
const char* PAL_GetChipName(void)
{
    return "STM32F072RBT6";
}

const char* PAL_GetCoreName(void)
{
    return "Arm Cortex-M0 (r0p0)";
}

uint32_t PAL_GetFlashSizeKB(void)
{
    uint16_t size = *(const uint16_t*)0x1FFFF7CC;
    return (size > 0 && size <= 1024) ? size : 128;
}

uint32_t PAL_GetRAMSizeKB(void)
{
    return 16;
}

uint32_t PAL_GetCPUID(void)
{
    return SCB->CPUID;
}

uint16_t PAL_GetDeviceID(void)
{
    return (uint16_t)(DBGMCU->IDCODE & 0x0FFF);
}

uint16_t PAL_GetRevisionID(void)
{
    return (uint16_t)(DBGMCU->IDCODE >> 16);
}

void PAL_GetUID(uint32_t uid[3])
{
    const uint32_t *uid_base = (const uint32_t *)0x1FFFF7AC;
    uid[0] = uid_base[0];
    uid[1] = uid_base[1];
    uid[2] = uid_base[2];
}

/* Board LED and Button */
void PAL_LED_Init(void) { BSP_LED_Init(); }
void PAL_LED_Set(int led_index, bool state)
{
    if (state) BSP_LED_On((uint8_t)led_index);
    else BSP_LED_Off((uint8_t)led_index);
}
void PAL_LED_Toggle(int led_index) { BSP_LED_Toggle((uint8_t)led_index); }
bool PAL_Button_Read(void) { return BSP_Button_IsPressed(); }
