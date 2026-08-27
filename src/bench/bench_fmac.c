/* STM32 FMAC (Filter Math Accelerator) Coprocessor Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include "stm32h7xx.h"
#include <stdint.h>

extern volatile uint32_t g_sink;

#define FMAC_TAPS    16
#define FMAC_SAMPLES 128

static int16_t s_fmac_coeffs[FMAC_TAPS] = {
    100, 250, 600, 1100, 1800, 2400, 2800, 3000,
    3000, 2800, 2400, 1800, 1100, 600, 250, 100
};
static int16_t s_fmac_in[FMAC_SAMPLES];
static int16_t s_fmac_out_sw[FMAC_SAMPLES];
static int16_t s_fmac_out_hw[FMAC_SAMPLES];

void Bench_FMAC_FIR(BenchResult *res)
{
    if (!PAL_HasFMAC()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    /* Enable FMAC clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_FMACEN;
    __DSB();

    for (int i = 0; i < FMAC_SAMPLES; i++) {
        s_fmac_in[i] = (int16_t)(i * 123);
    }

    const uint32_t runs = 1000;
    uint32_t sw_cycles = 0, hw_cycles = 0;

    /* 1. Software FIR Loop */
    {
        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t r = 0; r < runs; r++) {
            for (int i = 0; i < FMAC_SAMPLES; i++) {
                int32_t acc = 0;
                for (int k = 0; k < FMAC_TAPS; k++) {
                    if (i >= k) {
                        acc += (int32_t)s_fmac_coeffs[k] * (int32_t)s_fmac_in[i - k];
                    }
                }
                s_fmac_out_sw[i] = (int16_t)(acc >> 15);
            }
        }
        uint32_t t1 = PAL_GetCycleCount();
        sw_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = s_fmac_out_sw[0];
    }

    /* 2. Hardware FMAC */
    {
        /* Reset FMAC */
        FMAC->CR = FMAC_CR_RESET;
        FMAC->CR = 0;

        /* Configure buffers:
         * X1 buffer: input data (size = 32, base = 0)
         * X2 buffer: coefficients (size = 16, base = 32)
         * Y buffer: output data (size = 16, base = 48)
         */
        FMAC->X1BUFCFG = (0U << 0) | (32U << 8);
        FMAC->X2BUFCFG = (32U << 0) | ((uint32_t)FMAC_TAPS << 8);
        FMAC->YBUFCFG  = (48U << 0) | (16U << 8);

        /* Load coefficients into X2 */
        FMAC->PARAM = FMAC_PARAM_START | (0U << 16) | (FMAC_TAPS << 0); /* Load Coeffs */
        for (int k = 0; k < FMAC_TAPS; k++) {
            while (!(FMAC->SR & FMAC_SR_X1FULL)) { break; } /* Buffer check */
            FMAC->WDATA = (uint16_t)s_fmac_coeffs[k];
        }

        /* Start FIR Filter execution: FUNC = 0 (Convolution/FIR) */
        FMAC->PARAM = FMAC_PARAM_START | (0U << 24) | ((uint32_t)FMAC_TAPS << 0) | (0x7FU << 8);

        uint32_t t0 = PAL_GetCycleCount();
        for (uint32_t r = 0; r < runs; r++) {
            for (int i = 0; i < FMAC_SAMPLES; i++) {
                uint32_t to = 1000;
                while ((FMAC->SR & FMAC_SR_X1FULL) && --to) {}
                FMAC->WDATA = (uint16_t)s_fmac_in[i];

                to = 1000;
                while (!(FMAC->SR & FMAC_SR_YEMPTY) && --to) {
                    s_fmac_out_hw[i] = (int16_t)FMAC->RDATA;
                    break;
                }
            }
        }
        uint32_t t1 = PAL_GetCycleCount();
        hw_cycles = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1);
        g_sink = s_fmac_out_hw[0];
    }

    res->cycles = hw_cycles;
    res->time_us = (uint32_t)(((uint64_t)hw_cycles * 1000000ULL) / PAL_GetCoreClockHz());
    /* Acceleration speedup factor */
    res->score = (hw_cycles > 0) ? ((float)sw_cycles / (float)hw_cycles) : 1.0f;
}
