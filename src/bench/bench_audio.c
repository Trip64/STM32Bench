/* Audio & Signal Processing DSP Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include <math.h>
#include <string.h>

extern volatile uint32_t g_sink;

/* 1. Real FFT 512-pt with Hanning Window */
#define RFFT_LEN 512
static float s_audio_in[RFFT_LEN];
static float s_rfft_real[RFFT_LEN];
static float s_rfft_imag[RFFT_LEN];
static float s_window[RFFT_LEN];

void Bench_Audio_RFFT(BenchResult *res)
{
    /* Precompute Hanning window and audio test tone */
    for (int i = 0; i < RFFT_LEN; i++) {
        s_window[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(RFFT_LEN - 1)));
        s_audio_in[i] = sinf(2.0f * 3.14159265f * 440.0f * (float)i / 48000.0f);
    }

    const int iterations = 100;

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        /* Apply window */
        for (int i = 0; i < RFFT_LEN; i++) {
            s_rfft_real[i] = s_audio_in[i] * s_window[i];
            s_rfft_imag[i] = 0.0f;
        }

        /* Radix-2 Cooley-Tukey FFT */
        int j = 0;
        for (int i = 0; i < RFFT_LEN - 1; i++) {
            if (i < j) {
                float tr = s_rfft_real[i]; s_rfft_real[i] = s_rfft_real[j]; s_rfft_real[j] = tr;
                float ti = s_rfft_imag[i]; s_rfft_imag[i] = s_rfft_imag[j]; s_rfft_imag[j] = ti;
            }
            int k = RFFT_LEN >> 1;
            while (k <= j) { j -= k; k >>= 1; }
            j += k;
        }

        for (int len = 2; len <= RFFT_LEN; len <<= 1) {
            float ang = -2.0f * 3.14159265f / (float)len;
            float wlen_r = cosf(ang), wlen_i = sinf(ang);
            int half = len >> 1;

            for (int i = 0; i < RFFT_LEN; i += len) {
                float wr = 1.0f, wi = 0.0f;
                for (int m = 0; m < half; m++) {
                    float ur = s_rfft_real[i + m];
                    float ui = s_rfft_imag[i + m];
                    float vr = s_rfft_real[i + m + half] * wr - s_rfft_imag[i + m + half] * wi;
                    float vi = s_rfft_real[i + m + half] * wi + s_rfft_imag[i + m + half] * wr;

                    s_rfft_real[i + m] = ur + vr;
                    s_rfft_imag[i + m] = ui + vi;
                    s_rfft_real[i + m + half] = ur - vr;
                    s_rfft_imag[i + m + half] = ui - vi;

                    float next_wr = wr * wlen_r - wi * wlen_i;
                    wi = wr * wlen_i + wi * wlen_r;
                    wr = next_wr;
                }
            }
        }
    }
    g_sink = (uint32_t)(s_rfft_real[0] * 1000.0f);
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)iterations / time_sec / 1000.0f) : 0.0f; /* kFFT/s */
}

/* 2. 8-Stage Cascaded Direct Form II Transposed Biquad IIR Filter */
#define BIQUAD_STAGES 8
#define SAMPLES_PER_BLOCK 512

typedef struct {
    float b0, b1, b2, a1, a2;
    float d1, d2;
} BiquadStage;

static BiquadStage s_stages[BIQUAD_STAGES];
static float s_audio_block[SAMPLES_PER_BLOCK];

void Bench_Audio_Biquad(BenchResult *res)
{
    /* Initialize biquad coefficients for audio parametric EQ */
    for (int s = 0; s < BIQUAD_STAGES; s++) {
        s_stages[s].b0 = 0.2929f;
        s_stages[s].b1 = 0.5858f;
        s_stages[s].b2 = 0.2929f;
        s_stages[s].a1 = -0.0000f;
        s_stages[s].a2 = 0.1716f;
        s_stages[s].d1 = 0.0f;
        s_stages[s].d2 = 0.0f;
    }

    for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
        s_audio_block[i] = sinf((float)i * 0.1f);
    }

    const int iterations = 1000;

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        for (int s = 0; s < BIQUAD_STAGES; s++) {
            BiquadStage *bq = &s_stages[s];
            for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
                float in = s_audio_block[i];
                float out = in * bq->b0 + bq->d1;
                bq->d1 = in * bq->b1 - out * bq->a1 + bq->d2;
                bq->d2 = in * bq->b2 - out * bq->a2;
                s_audio_block[i] = out;
            }
        }
    }
    g_sink = (uint32_t)(s_audio_block[0] * 1000.0f);
    BENCH_STOP();

    /* Total filtered samples across all passes */
    float total_samples = (float)iterations * (float)SAMPLES_PER_BLOCK;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_samples / time_sec / 1000000.0f) : 0.0f; /* MSamples/s */
}
