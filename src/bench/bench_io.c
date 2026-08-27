/* GPIO, Peripheral Bus & DMA Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#if defined(STM32F0) || defined(STM32F072xB)
  #include "stm32f072xb.h"
  #define BENCH_GPIO_RCC_EN()   do { RCC->AHBENR |= RCC_AHBENR_GPIOAEN; __DSB(); } while(0)
  #define BENCH_GPIO_SETUP()    do { \
                                  GPIOA->MODER = (GPIOA->MODER & ~(3U << (5 * 2))) | (1U << (5 * 2)); \
                                  GPIOA->OTYPER &= ~(1U << 5); \
                                  GPIOA->OSPEEDR |= (3U << (5 * 2)); \
                                } while(0)
  #define BENCH_GPIO_PORT       GPIOA
  #define BENCH_GPIO_BS         (1U << 5)
  #define BENCH_GPIO_BR         (1U << (5 + 16))
#else
  #include "stm32h7xx.h"
  #define BENCH_GPIO_RCC_EN()   do { RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN; __DSB(); } while(0)
  #define BENCH_GPIO_SETUP()    do { \
                                  GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE0_Msk) | (1U << GPIO_MODER_MODE0_Pos); \
                                  GPIOB->OTYPER &= ~GPIO_OTYPER_OT0; \
                                  GPIOB->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED0_Pos); \
                                } while(0)
  #define BENCH_GPIO_PORT       GPIOB
  #define BENCH_GPIO_BS         GPIO_BSRR_BS0
  #define BENCH_GPIO_BR         GPIO_BSRR_BR0
#endif
#include <string.h>

extern volatile uint32_t g_sink;

/* 1. Atomic GPIO Pin Toggle Frequency (BSRR Register) */
void Bench_IO_GPIOToggle(BenchResult *res)
{
    BENCH_GPIO_RCC_EN();
    BENCH_GPIO_SETUP();

    const uint32_t iterations = 50000;
    /* 16 toggles per unrolled loop iteration */
    const uint64_t total_toggles = (uint64_t)iterations * 16ULL;

    BENCH_START();
    for (uint32_t i = 0; i < iterations; i++) {
        /* Unrolled atomic bit set/reset */
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
        BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BS; BENCH_GPIO_PORT->BSRR = BENCH_GPIO_BR;
    }
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)total_toggles / time_sec / 1000000.0f) : 0.0f; /* MToggles/s (MHz) */
}

/* 2. GPIO Input Polling Read Bandwidth (IDR Register) */
void Bench_IO_GPIORead(BenchResult *res)
{
    BENCH_GPIO_RCC_EN();

    const uint32_t iterations = 50000;
    const uint64_t total_reads = (uint64_t)iterations * 16ULL;
    uint32_t acc = 0;

    BENCH_START();
    for (uint32_t i = 0; i < iterations; i++) {
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
        acc += BENCH_GPIO_PORT->IDR; acc += BENCH_GPIO_PORT->IDR;
    }
    g_sink = acc;
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)total_reads / time_sec / 1000000.0f) : 0.0f; /* MReads/s */
}

#if BENCH_SUITE_EXTENDED && (defined(STM32H7) || defined(STM32H723xx))
/* 3. DMA Memory-to-Memory Transfer Throughput (DMA1 Stream 0 in D2 SRAM) */
#define DMA_BUF_WORDS 1024
static uint32_t s_dma_src[DMA_BUF_WORDS] __attribute__((section(".d2_sram"), aligned(32)));
static uint32_t s_dma_dst[DMA_BUF_WORDS] __attribute__((section(".d2_sram"), aligned(32)));

void Bench_IO_DMAM2M(BenchResult *res)
{
    for (int i = 0; i < DMA_BUF_WORDS; i++) s_dma_src[i] = (uint32_t)(i * 101);

    /* Enable DMA1 clock on AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    __DSB();

    DMA1_Stream0->CR = 0; /* Disable stream */
    uint32_t to = 10000;
    while ((DMA1_Stream0->CR & DMA_SxCR_EN) && --to) {}

    /* Clear all interrupt flags for Stream 0 */
    DMA1->LIFCR = 0x3D;

    /* Configure DMA1 Stream 0 for Memory-to-Memory (Direction = 10b) */
    DMA1_Stream0->PAR  = (uint32_t)s_dma_src;
    DMA1_Stream0->M0AR = (uint32_t)s_dma_dst;
    DMA1_Stream0->NDTR = DMA_BUF_WORDS;
    DMA1_Stream0->CR   = (2U << DMA_SxCR_DIR_Pos)      /* Memory-to-Memory */
                       | (2U << DMA_SxCR_MSIZE_Pos)    /* 32-bit Memory */
                       | (2U << DMA_SxCR_PSIZE_Pos)    /* 32-bit Peripheral */
                       | DMA_SxCR_MINC                 /* Memory increment */
                       | DMA_SxCR_PINC                 /* Peripheral increment */
                       | (3U << DMA_SxCR_PL_Pos);      /* Very high priority */

    const int transfers = 500; /* 500 x 4KB = 2 MB */

    BENCH_START();
    for (int t = 0; t < transfers; t++) {
        DMA1_Stream0->CR &= ~DMA_SxCR_EN;
        DMA1->LIFCR = 0x3D;
        DMA1_Stream0->NDTR = DMA_BUF_WORDS;
        DMA1_Stream0->CR |= DMA_SxCR_EN;

        uint32_t timeout = 50000;
        while (!(DMA1->LISR & DMA_LISR_TCIF0) && --timeout) {
            if (DMA1->LISR & (DMA_LISR_TEIF0 | DMA_LISR_FEIF0)) {
                DMA1->LIFCR = 0x3D;
                break;
            }
        }
        if (!timeout) break;
    }
    g_sink = s_dma_dst[0];
    BENCH_STOP();

    float bytes = (float)(transfers * DMA_BUF_WORDS * 4);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}
#else
void Bench_IO_DMAM2M(BenchResult *res)
{
    res->available = false;
    res->score = 0.0f;
}
#endif
