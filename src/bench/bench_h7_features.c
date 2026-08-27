/* Advanced Cortex-M7 & STM32H7 Hardware Feature Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include <string.h>

#if defined(STM32H723xx) || defined(STM32H7)
#include "stm32h7xx.h"

extern volatile uint32_t g_sink;

/* ── 1. ITCM 0-Wait-State Execution Benchmark ───────────────────────── */
/* Demonstrates zero-wait-state instruction execution from 64 KB ITCM vs Flash */
__attribute__((section(".itcm_text"), noinline))
static uint32_t kernel_in_itcm(uint32_t iters)
{
    uint32_t a = 0x12345678, b = 0x87654321;
    for (uint32_t i = 0; i < iters; i++) {
        a = (a ^ (b >> 3)) + 0x9E3779B9 + (a << 6);
        b = (b ^ (a >> 2)) + 0x85EBCA6B + (b << 4);
    }
    return a ^ b;
}

__attribute__((noinline))
static uint32_t kernel_in_flash(uint32_t iters)
{
    uint32_t a = 0x12345678, b = 0x87654321;
    for (uint32_t i = 0; i < iters; i++) {
        a = (a ^ (b >> 3)) + 0x9E3779B9 + (a << 6);
        b = (b ^ (a >> 2)) + 0x85EBCA6B + (b << 4);
    }
    return a ^ b;
}

void Bench_Mem_ITCM(BenchResult *res)
{
    const uint32_t iters = 200000;

    /* Benchmark Flash execution */
    uint32_t cyc_flash_start = DWT->CYCCNT;
    g_sink = kernel_in_flash(iters);
    uint32_t cyc_flash = DWT->CYCCNT - cyc_flash_start;

    /* Benchmark ITCM 0-wait-state execution */
    BENCH_START();
    g_sink = kernel_in_itcm(iters);
    BENCH_STOP();

    float speedup = (res->cycles > 0) ? ((float)cyc_flash / (float)res->cycles) : 1.0f;
    /* Reported in speedup ratio vs Flash execution */
    res->score = (speedup >= 1.0f) ? speedup : 1.0f;
}

/* ── 2. L1 Cache Line Hit/Miss & Stride Throughput ──────────────────── */
/* Measures 32-byte cache line fill behavior in AXI-SRAM */
#define STRIDE_BUF_WORDS 4096 /* 16 KB buffer */
static uint32_t s_stride_buf[STRIDE_BUF_WORDS] __attribute__((aligned(32)));

void Bench_Mem_CacheStride(BenchResult *res)
{
    for (int i = 0; i < STRIDE_BUF_WORDS; i++) s_stride_buf[i] = (uint32_t)(i ^ 0x55AA55AA);

    const int passes = 1500;
    uint32_t sum = 0;

    BENCH_START();
    /* Strided access with 32-byte step (8 words): each read is on a new L1 cache line */
    for (int p = 0; p < passes; p++) {
        for (int i = 0; i < STRIDE_BUF_WORDS; i += 8) {
            sum += s_stride_buf[i];
            sum += s_stride_buf[i + 1];
        }
    }
    g_sink = sum;
    BENCH_STOP();

    float bytes = (float)(passes * (STRIDE_BUF_WORDS / 4) * 4);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

/* ── 3. On-Chip Hardware CRC-32 Engine (AHB4) ───────────────────────── */
/* Measures dedicated hardware CRC unit vs software calculation */
#define CRC_DATA_WORDS 2048 /* 8 KB */
static uint32_t s_crc_data[CRC_DATA_WORDS];

void Bench_CPU_HwCRC(BenchResult *res)
{
    for (int i = 0; i < CRC_DATA_WORDS; i++) s_crc_data[i] = (uint32_t)(i * 37 + 1);

    /* Enable Hardware CRC clock on AHB4 */
    RCC->AHB4ENR |= RCC_AHB4ENR_CRCEN;
    __DSB();

    /* Reset CRC calculation unit */
    CRC->CR = CRC_CR_RESET;

    const int passes = 50; /* 50 x 8 KB = 400 KB */

    BENCH_START();
    for (int p = 0; p < passes; p++) {
        CRC->CR = CRC_CR_RESET;
        for (int i = 0; i < CRC_DATA_WORDS; i += 4) {
            CRC->DR = s_crc_data[i];
            CRC->DR = s_crc_data[i + 1];
            CRC->DR = s_crc_data[i + 2];
            CRC->DR = s_crc_data[i + 3];
        }
    }
    g_sink = CRC->DR;
    BENCH_STOP();

    float bytes = (float)(passes * CRC_DATA_WORDS * 4);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

/* ── 4. 64-bit Master DMA (MDMA Channel 0) on AXI-SRAM ──────────────── */
/* Dedicated 64-bit bus master block copy */
#define MDMA_WORDS 1024
static uint32_t s_mdma_src[MDMA_WORDS] __attribute__((aligned(32)));
static uint32_t s_mdma_dst[MDMA_WORDS] __attribute__((aligned(32)));

void Bench_IO_MDMA(BenchResult *res)
{
    for (int i = 0; i < MDMA_WORDS; i++) s_mdma_src[i] = (uint32_t)(i * 101);

    /* Enable MDMA clock on AHB3 */
    RCC->AHB3ENR |= RCC_AHB3ENR_MDMAEN;
    __DSB();

    MDMA_Channel0->CCR = 0; /* Disable channel */
    uint32_t to = 10000;
    while ((MDMA_Channel0->CCR & MDMA_CCR_EN) && --to) {}

    /* Clear all status flags */
    MDMA_Channel0->CIFCR = 0x1F;

    /* Configure 64-bit transfer with increment */
    MDMA_Channel0->CTCR = (3U << MDMA_CTCR_SSIZE_Pos)  /* 64-bit source */
                        | (3U << MDMA_CTCR_DSIZE_Pos)  /* 64-bit dest */
                        | (2U << MDMA_CTCR_SINC_Pos)   /* Source increment */
                        | (2U << MDMA_CTCR_DINC_Pos);  /* Dest increment */

    MDMA_Channel0->CSAR = (uint32_t)s_mdma_src;
    MDMA_Channel0->CDAR = (uint32_t)s_mdma_dst;

    const int transfers = 600; /* 600 x 4 KB = 2.4 MB */

    BENCH_START();
    for (int t = 0; t < transfers; t++) {
        MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
        MDMA_Channel0->CIFCR = 0x1F;
        MDMA_Channel0->CBNDTR = (MDMA_WORDS * 4); /* Total bytes */
        MDMA_Channel0->CCR |= MDMA_CCR_EN | MDMA_CCR_SWRQ;

        uint32_t timeout = 50000;
        while (!(MDMA_Channel0->CISR & MDMA_CISR_CTCIF) && --timeout) {
            if (MDMA_Channel0->CISR & MDMA_CISR_TEIF) {
                MDMA_Channel0->CIFCR = 0x1F;
                break;
            }
        }
        if (!timeout) break;
    }
    g_sink = s_mdma_dst[0];
    BENCH_STOP();

    float bytes = (float)(transfers * MDMA_WORDS * 4);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

#endif /* STM32H7 */
