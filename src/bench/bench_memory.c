/* Memory Subsystem Benchmarks (Bandwidth, Latency, Cache Impact) */

#include "bench_engine.h"
#include "pal.h"
#include <string.h>

extern volatile uint32_t g_sink;

#if BENCH_SUITE_EXTENDED
#define BLOCK_SIZE (16 * 1024) /* 16 KB buffers for high-end MCUs */
#else
#define BLOCK_SIZE (1024)      /* 1 KB buffers for 16 KB RAM MCUs */
#endif

static uint32_t s_mem_src[BLOCK_SIZE / 4] __attribute__((aligned(32)));
static uint32_t s_mem_dst[BLOCK_SIZE / 4] __attribute__((aligned(32)));

#if BENCH_SUITE_EXTENDED && (defined(STM32H7) || defined(STM32H723xx))
/* Constant table in Flash */
static const uint32_t s_flash_table[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
};

/* Table in DTCM section */
static uint32_t s_dtcm_table[256] __attribute__((section(".dtcm")));
#endif

/* 1. Memcpy Bandwidth (AXI SRAM) */
void Bench_Mem_Memcpy(BenchResult *res)
{
    for (size_t i = 0; i < BLOCK_SIZE / 4; i++) {
        s_mem_src[i] = (uint32_t)(i * 12345);
    }

    const uint32_t runs = 100; /* 100 * 16KB = 1.6 MB */

    BENCH_START();
    for (uint32_t r = 0; r < runs; r++) {
        memcpy(s_mem_dst, s_mem_src, BLOCK_SIZE);
        s_mem_src[0] = s_mem_dst[0] + 1; /* prevent dead store */
    }
    g_sink = s_mem_dst[0];
    BENCH_STOP();

    float total_bytes = (float)runs * (float)BLOCK_SIZE;
    float total_mb = total_bytes / (1024.0f * 1024.0f);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_mb / time_sec) : 0.0f; /* MB/s */
}

/* 2. Memset Bandwidth */
void Bench_Mem_Memset(BenchResult *res)
{
    const uint32_t runs = 100; /* 100 * 16KB = 1.6 MB */

    BENCH_START();
    for (uint32_t r = 0; r < runs; r++) {
        memset(s_mem_dst, (int)(r & 0xFF), BLOCK_SIZE);
    }
    g_sink = s_mem_dst[0];
    BENCH_STOP();

    float total_bytes = (float)runs * (float)BLOCK_SIZE;
    float total_mb = total_bytes / (1024.0f * 1024.0f);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_mb / time_sec) : 0.0f; /* MB/s */
}

#if BENCH_SUITE_EXTENDED && (defined(STM32H7) || defined(STM32H723xx))
/* 3. DTCM vs Flash Access Latency */
void Bench_Mem_DTCMvsFlash(BenchResult *res)
{
    /* Initialize DTCM table */
    for (int i = 0; i < 256; i++) {
        s_dtcm_table[i] = s_flash_table[i];
    }

    const uint32_t iterations = 5000;
    uint32_t flash_cycles = 0, dtcm_cycles = 0;

    /* Read from Flash (7 wait states) */
    {
        uint32_t sum = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < iterations; i++) {
            sum += s_flash_table[i & 0xFF];
        }
        flash_cycles = PAL_GetCycleCount() - t0;
        g_sink = sum;
    }

    /* Read from 0-wait state DTCM */
    {
        uint32_t sum = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < iterations; i++) {
            sum += s_dtcm_table[i & 0xFF];
        }
        dtcm_cycles = PAL_GetCycleCount() - t0;
        g_sink = sum;
    }

    res->cycles = flash_cycles;
    res->time_us = (uint32_t)((uint64_t)flash_cycles * 1000000ULL / PAL_GetCoreClockHz());
    /* Ratio: how much faster DTCM is compared to Flash */
    res->score = (dtcm_cycles > 0) ? ((float)flash_cycles / (float)dtcm_cycles) : 1.0f;
}

/* 4. D-Cache Impact */
void Bench_Mem_CacheImpact(BenchResult *res)
{
    const uint32_t iterations = 5000;
    uint32_t uncached_cycles = 0, cached_cycles = 0;

    /* Measure with D-Cache ON */
    {
        PAL_EnableDCache();
        uint32_t sum = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < iterations; i++) {
            sum += s_mem_src[(i * 17) & ((BLOCK_SIZE / 4) - 1)];
        }
        uint32_t t1 = PAL_GetCycleCount();
        cached_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = sum;
    }

    /* Measure with D-Cache OFF */
    {
        PAL_CleanInvalidateDCache();
        PAL_DisableDCache();
        uint32_t sum = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < iterations; i++) {
            sum += s_mem_src[(i * 17) & ((BLOCK_SIZE / 4) - 1)];
        }
        uint32_t t1 = PAL_GetCycleCount();
        uncached_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = sum;

        /* Restore D-Cache ON */
        PAL_EnableDCache();
    }

    res->cycles = cached_cycles;
    res->time_us = (uint32_t)(((uint64_t)cached_cycles * 1000000ULL) / PAL_GetCoreClockHz());
    /* Speedup factor with cache enabled */
    res->score = (cached_cycles > 0) ? ((float)uncached_cycles / (float)cached_cycles) : 1.0f;
}
#else
void Bench_Mem_DTCMvsFlash(BenchResult *res) { res->available = false; res->score = 0.0f; }
void Bench_Mem_CacheImpact(BenchResult *res) { res->available = false; res->score = 0.0f; }
#endif
