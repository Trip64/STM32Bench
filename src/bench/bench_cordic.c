/* STM32 Hardware CORDIC Coprocessor Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include "stm32h7xx.h"
#include <math.h>

extern volatile uint32_t g_sink;

/* Helper to convert float angle [-PI, PI] to CORDIC Q1.31 format [-1.0, 1.0) */
static inline int32_t float_to_q31(float val)
{
    return (int32_t)(val * 2147483648.0f);
}

/* 1. CORDIC Sine/Cosine Hardware vs Software Benchmark */
void Bench_CORDIC_SinCos(BenchResult *res)
{
    if (!PAL_HasCORDIC()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    /* Enable CORDIC clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_CORDICEN;
    __DSB();

    const uint32_t samples = 10000;
    uint32_t soft_cycles = 0;
    uint32_t hard_cycles = 0;

    /* Benchmark Software sinf + cosf */
    {
        float s = 0.0f, c = 0.0f;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < samples; i++) {
            float angle = -3.14159f + ((float)i / (float)samples) * 6.28318f;
            s += sinf(angle);
            c += cosf(angle);
        }
        uint32_t t1 = PAL_GetCycleCount();
        soft_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = (uint32_t)(s + c);
    }

    /* Benchmark Hardware CORDIC (both Sin and Cos simultaneously in 1 pass) */
    {
        /* Configure CORDIC:
         * FUNC = 0 (Cosine, or 0 with NRES=1 gives Cosine in RDATA1, Sine in RDATA2)
         * PRECISION = 6 (24 iterations)
         * NRES = 1 (2 results)
         * NARGS = 0 (1 arg: angle)
         * RSIZE = 0 (32-bit)
         * ARGSIZE = 0 (32-bit)
         */
        CORDIC->CSR = (0U << 0)  |  /* FUNC: Cosine */
                      (6U << 4)  |  /* PRECISION: 24 cycles */
                      (1U << 16) |  /* NRES: 2 results */
                      (0U << 20) |  /* NARGS: 1 argument */
                      (0U << 21) |  /* ARGSIZE: 32-bit */
                      (0U << 22);   /* RSIZE: 32-bit */

        int32_t cos_res = 0, sin_res = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < samples; i++) {
            /* Angle in semi-circles: -1.0 to +1.0 in Q1.31 */
            int32_t q31_angle = (int32_t)(-2147483648LL + (int64_t)i * (4294967295LL / samples));
            CORDIC->WDATA = (uint32_t)q31_angle;
            uint32_t to = 5000;
            while (!(CORDIC->CSR & CORDIC_CSR_RRDY) && --to) {}
            cos_res = (int32_t)CORDIC->RDATA;
            sin_res = (int32_t)CORDIC->RDATA;
        }
        uint32_t t1 = PAL_GetCycleCount();
        hard_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = (uint32_t)(cos_res + sin_res);
    }

    res->cycles = hard_cycles;
    res->time_us = (uint32_t)(((uint64_t)hard_cycles * 1000000ULL) / PAL_GetCoreClockHz());
    /* Score is hardware speedup factor over software */
    res->score = (hard_cycles > 0) ? ((float)soft_cycles / (float)hard_cycles) : 1.0f;
}

/* 2. CORDIC Atan2 Hardware vs Software Benchmark */
void Bench_CORDIC_Atan2(BenchResult *res)
{
    if (!PAL_HasCORDIC()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    RCC->AHB2ENR |= RCC_AHB2ENR_CORDICEN;
    __DSB();

    const uint32_t samples = 10000;
    uint32_t soft_cycles = 0, hard_cycles = 0;

    /* Software atan2f */
    {
        float sum = 0.0f;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < samples; i++) {
            float y = -1.0f + ((float)i / (float)samples) * 2.0f;
            float x = 0.5f;
            sum += atan2f(y, x);
        }
        uint32_t t1 = PAL_GetCycleCount();
        soft_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = (uint32_t)sum;
    }

    /* Hardware CORDIC Phase (atan2) */
    {
        /* FUNC = 2 (Phase / Atan2)
         * NARGS = 1 (2 args: x and y)
         * NRES = 0 (1 result: angle)
         */
        CORDIC->CSR = (2U << 0)  |  /* FUNC: Phase */
                      (6U << 4)  |  /* PRECISION: 24 cycles */
                      (0U << 16) |  /* NRES: 1 result */
                      (1U << 20) |  /* NARGS: 2 arguments */
                      (0U << 21) |  /* ARGSIZE: 32-bit */
                      (0U << 22);   /* RSIZE: 32-bit */

        int32_t phase = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 0; i < samples; i++) {
            int32_t y = (int32_t)(-1073741824 + (int64_t)i * (2147483648LL / samples));
            int32_t x = 1073741824; /* 0.5 in Q1.31 */
            CORDIC->WDATA = (uint32_t)x;
            CORDIC->WDATA = (uint32_t)y;
            uint32_t to = 5000;
            while (!(CORDIC->CSR & CORDIC_CSR_RRDY) && --to) {}
            phase = (int32_t)CORDIC->RDATA;
        }
        uint32_t t1 = PAL_GetCycleCount();
        hard_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = (uint32_t)phase;
    }

    res->cycles = hard_cycles;
    res->time_us = (uint32_t)(((uint64_t)hard_cycles * 1000000ULL) / PAL_GetCoreClockHz());
    res->score = (hard_cycles > 0) ? ((float)soft_cycles / (float)hard_cycles) : 1.0f;
}

/* 3. CORDIC Square Root vs Software Sqrt */
void Bench_CORDIC_Sqrt(BenchResult *res)
{
    if (!PAL_HasCORDIC()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    RCC->AHB2ENR |= RCC_AHB2ENR_CORDICEN;
    __DSB();

    const uint32_t samples = 10000;
    uint32_t soft_cycles = 0, hard_cycles = 0;

    /* Software sqrtf */
    {
        float sum = 0.0f;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 1; i <= samples; i++) {
            float val = (float)i * 0.0001f;
            sum += sqrtf(val);
        }
        uint32_t t1 = PAL_GetCycleCount();
        soft_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = (uint32_t)sum;
    }

    /* Hardware CORDIC Sqrt: FUNC = 9 */
    {
        CORDIC->CSR = (9U << 0)  |  /* FUNC: Square root */
                      (6U << 4)  |  /* PRECISION: 24 cycles */
                      (0U << 16) |  /* NRES: 1 result */
                      (0U << 20) |  /* NARGS: 1 argument */
                      (0U << 21) |  /* ARGSIZE: 32-bit */
                      (0U << 22);   /* RSIZE: 32-bit */

        uint32_t root = 0;
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t i = 1; i <= samples; i++) {
            uint32_t arg = (uint32_t)((uint64_t)i * 2147483648ULL / samples);
            CORDIC->WDATA = arg;
            uint32_t to = 5000;
            while (!(CORDIC->CSR & CORDIC_CSR_RRDY) && --to) {}
            root = CORDIC->RDATA;
        }
        uint32_t t1 = PAL_GetCycleCount();
        hard_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = root;
    }

    res->cycles = hard_cycles;
    res->time_us = (uint32_t)(((uint64_t)hard_cycles * 1000000ULL) / PAL_GetCoreClockHz());
    res->score = (hard_cycles > 0) ? ((float)soft_cycles / (float)hard_cycles) : 1.0f;
}
