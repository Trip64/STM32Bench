/* DSP Extension & SIMD Benchmarks (Q15 fixed-point) */

#include "bench_engine.h"
#include "pal.h"
#if defined(STM32F0) || defined(STM32F072xB)
  #include "stm32f072xb.h"
#else
  #include "stm32h7xx.h"
#endif
#include <stdint.h>
#include <string.h>

extern volatile uint32_t g_sink;

#define VEC_LEN 512
static int16_t s_vec_a[VEC_LEN] __attribute__((aligned(4)));
static int16_t s_vec_b[VEC_LEN] __attribute__((aligned(4)));

/* 1. Vector Dot Product using ARM DSP SIMD (__SMLAD) or portable scalar MAC */
void Bench_DSP_DotProduct(BenchResult *res)
{
    /* Initialize test vectors with Q15 values */
    for (int i = 0; i < VEC_LEN; i++) {
        s_vec_a[i] = (int16_t)(i * 53);
        s_vec_b[i] = (int16_t)((VEC_LEN - i) * 31);
    }

    const uint32_t runs = 5000;
    int32_t sum = 0;

    BENCH_START();
    for (uint32_t r = 0; r < runs; r++) {
        sum = 0;
#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
        const uint32_t *pA = (const uint32_t *)s_vec_a;
        const uint32_t *pB = (const uint32_t *)s_vec_b;
        int count = VEC_LEN / 2;

        /* Dual 16-bit MAC loop using Cortex-M7 SIMD __SMLAD */
        while (count > 0) {
            uint32_t a = *pA++;
            uint32_t b = *pB++;
            sum = __SMLAD(a, b, sum);
            count--;
        }
#else
        for (int i = 0; i < VEC_LEN; i++) {
            sum += (int32_t)s_vec_a[i] * s_vec_b[i];
        }
#endif
    }
    g_sink = sum;
    BENCH_STOP();

    res->available = PAL_HasDSP();

    /* VEC_LEN MAC operations per run */
    float total_macs = (float)runs * (float)VEC_LEN;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_macs / time_sec / 1000000.0f) : 0.0f; /* MMAC/s */
}

/* 2. FIR Filter (32-tap Q15, 256 samples) */
#define FIR_TAPS    32
#define FIR_SAMPLES 256
static int16_t s_fir_coeffs[FIR_TAPS] __attribute__((aligned(4)));
static int16_t s_fir_input[FIR_SAMPLES] __attribute__((aligned(4)));
static int16_t s_fir_output[FIR_SAMPLES] __attribute__((aligned(4)));
static int16_t s_fir_history[FIR_TAPS] __attribute__((aligned(4)));

void Bench_DSP_FIR(BenchResult *res)
{
    /* Initialize coeffs & input */
    for (int i = 0; i < FIR_TAPS; i++)    s_fir_coeffs[i] = (int16_t)(1000 - i * 30);
    for (int i = 0; i < FIR_SAMPLES; i++) s_fir_input[i]  = (int16_t)(i * 40 - 5000);
    memset(s_fir_history, 0, sizeof(s_fir_history));

    const uint32_t passes = 500;

    BENCH_START();
    for (uint32_t p = 0; p < passes; p++) {
        for (int n = 0; n < FIR_SAMPLES; n++) {
            /* Shift history buffer */
            for (int k = FIR_TAPS - 1; k > 0; k--) {
                s_fir_history[k] = s_fir_history[k - 1];
            }
            s_fir_history[0] = s_fir_input[n];

            int32_t sum = 0;
#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
            const uint32_t *pH = (const uint32_t *)s_fir_history;
            const uint32_t *pC = (const uint32_t *)s_fir_coeffs;
            for (int k = 0; k < FIR_TAPS / 2; k++) {
                uint32_t x_dual = *pH++;
                uint32_t c_dual = *pC++;
                sum = __SMLAD(x_dual, c_dual, sum);
            }
#else
            for (int k = 0; k < FIR_TAPS; k++) {
                sum += (int32_t)s_fir_history[k] * (int32_t)s_fir_coeffs[k];
            }
#endif
            s_fir_output[n] = (int16_t)(sum >> 15);
        }
    }
    g_sink = s_fir_output[0];
    BENCH_STOP();

    res->available = PAL_HasDSP();

    float total_samples = (float)passes * (float)FIR_SAMPLES;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_samples / time_sec / 1000000.0f) : 0.0f; /* MSamples/s */
}

#if BENCH_SUITE_EXTENDED
/* 3. Vector Magnitude (Q15 Complex) */
static int16_t s_cplx_re[256];
static int16_t s_cplx_im[256];
static int16_t s_cplx_mag[256];

/* Fast integer square root */
static uint32_t isqrt32(uint32_t val)
{
    uint32_t res = 0;
    uint32_t bit = 1UL << 30;
    while (bit > val) bit >>= 2;
    while (bit != 0) {
        if (val >= res + bit) {
            val -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

void Bench_DSP_VectorMag(BenchResult *res)
{
    for (int i = 0; i < 256; i++) {
        s_cplx_re[i] = (int16_t)(i * 40);
        s_cplx_im[i] = (int16_t)((256 - i) * 30);
    }

    const uint32_t passes = 1000;

    BENCH_START();
    for (uint32_t p = 0; p < passes; p++) {
        for (int i = 0; i < 256; i++) {
            int32_t re = (int32_t)s_cplx_re[i];
            int32_t im = (int32_t)s_cplx_im[i];
            uint32_t mag_sq = (uint32_t)(re * re + im * im);
            s_cplx_mag[i] = (int16_t)isqrt32(mag_sq);
        }
    }
    g_sink = s_cplx_mag[0];
    BENCH_STOP();

    float total_vectors = (float)passes;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_vectors / time_sec / 1000.0f) : 0.0f; /* kVectors/s */
}
#else
void Bench_DSP_VectorMag(BenchResult *res)
{
    res->available = false;
    res->score = 0.0f;
}
#endif
