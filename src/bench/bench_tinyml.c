/* TinyML & Edge AI Neural Network Inference Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include <math.h>
#include <string.h>

extern volatile uint32_t g_sink;

/* 1. Quantized 2D Convolution Layer (int8 weights & activations)
 * 16x16 input, 3x3 kernel, 8 output channels, stride 1, padding 1
 */
#define CONV_IN_DIM     16
#define CONV_IN_CH      4
#define CONV_OUT_CH     8
#define CONV_KERNEL     3

static int8_t s_conv_in[CONV_IN_DIM * CONV_IN_DIM * CONV_IN_CH];
static int8_t s_conv_out[CONV_IN_DIM * CONV_IN_DIM * CONV_OUT_CH];
static int8_t s_conv_weights[CONV_OUT_CH * CONV_KERNEL * CONV_KERNEL * CONV_IN_CH];
static int32_t s_conv_bias[CONV_OUT_CH];

void Bench_ML_Conv2D(BenchResult *res)
{
    /* Initialize test tensors */
    for (int i = 0; i < (int)sizeof(s_conv_in); i++) s_conv_in[i] = (int8_t)(i * 3);
    for (int i = 0; i < (int)sizeof(s_conv_weights); i++) s_conv_weights[i] = (int8_t)(i * 7);
    for (int i = 0; i < CONV_OUT_CH; i++) s_conv_bias[i] = i * 16;

    const int iterations = 10;
    /* Total MACs per convolution: (16*16) * (3*3*4) * 8 = 73,728 MACs per pass */
    const uint64_t total_macs = (uint64_t)iterations * (16 * 16 * 3 * 3 * CONV_IN_CH * CONV_OUT_CH);

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        for (int out_y = 0; out_y < CONV_IN_DIM; out_y++) {
            for (int out_x = 0; out_x < CONV_IN_DIM; out_x++) {
                for (int oc = 0; oc < CONV_OUT_CH; oc++) {
                    int32_t acc = s_conv_bias[oc];

                    for (int ky = 0; ky < CONV_KERNEL; ky++) {
                        int in_y = out_y + ky - 1;
                        if (in_y < 0 || in_y >= CONV_IN_DIM) continue;

                        for (int kx = 0; kx < CONV_KERNEL; kx++) {
                            int in_x = out_x + kx - 1;
                            if (in_x < 0 || in_x >= CONV_IN_DIM) continue;

                            int in_idx = (in_y * CONV_IN_DIM + in_x) * CONV_IN_CH;
                            int w_idx  = ((oc * CONV_KERNEL + ky) * CONV_KERNEL + kx) * CONV_IN_CH;

                            for (int ic = 0; ic < CONV_IN_CH; ic++) {
                                acc += (int32_t)s_conv_in[in_idx + ic] * (int32_t)s_conv_weights[w_idx + ic];
                            }
                        }
                    }

                    /* Quantized activation scaling & ReLU */
                    acc >>= 6;
                    if (acc < 0) acc = 0;
                    if (acc > 127) acc = 127;
                    s_conv_out[(out_y * CONV_IN_DIM + out_x) * CONV_OUT_CH + oc] = (int8_t)acc;
                }
            }
        }
    }
    g_sink = s_conv_out[0];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)total_macs / time_sec / 1000000.0f) : 0.0f; /* MMAC/s */
}

/* 2. Quantized Fully-Connected (Dense) Layer
 * 64 inputs -> 16 outputs with bias & fixed-point scaling
 */
#define DENSE_IN_DIM   64
#define DENSE_OUT_DIM  16

static int8_t s_dense_in[DENSE_IN_DIM];
static int8_t s_dense_out[DENSE_OUT_DIM];
static int8_t s_dense_weights[DENSE_OUT_DIM * DENSE_IN_DIM];
static int32_t s_dense_bias[DENSE_OUT_DIM];

void Bench_ML_Dense(BenchResult *res)
{
    for (int i = 0; i < DENSE_IN_DIM; i++) s_dense_in[i] = (int8_t)(i * 5 - 50);
    for (int i = 0; i < DENSE_OUT_DIM * DENSE_IN_DIM; i++) s_dense_weights[i] = (int8_t)(i * 3 - 30);
    for (int i = 0; i < DENSE_OUT_DIM; i++) s_dense_bias[i] = i * 32;

    const int inferences = 5000;

    BENCH_START();
    for (int inf = 0; inf < inferences; inf++) {
        s_dense_in[0] = (int8_t)inf;

        for (int o = 0; o < DENSE_OUT_DIM; o++) {
            int32_t acc = s_dense_bias[o];
            const int8_t *w_row = &s_dense_weights[o * DENSE_IN_DIM];

            /* Cortex-M7 dual-issue unrolled dot product */
            for (int i = 0; i < DENSE_IN_DIM; i += 4) {
                acc += (int32_t)s_dense_in[i + 0] * (int32_t)w_row[i + 0];
                acc += (int32_t)s_dense_in[i + 1] * (int32_t)w_row[i + 1];
                acc += (int32_t)s_dense_in[i + 2] * (int32_t)w_row[i + 2];
                acc += (int32_t)s_dense_in[i + 3] * (int32_t)w_row[i + 3];
            }

            acc >>= 7;
            if (acc < -128) acc = -128;
            if (acc > 127) acc = 127;
            s_dense_out[o] = (int8_t)acc;
        }
    }
    g_sink = s_dense_out[0];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)inferences / time_sec / 1000.0f) : 0.0f; /* kInf/s */
}

/* 3. Vectorized Softmax Layer (16 classes) */
static float s_logits[16];
static float s_probabilities[16];

void Bench_ML_Softmax(BenchResult *res)
{
    for (int i = 0; i < 16; i++) s_logits[i] = (float)i * 0.35f - 2.5f;

    const int iterations = 10000;

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        s_logits[0] = (float)(iter & 0x0F) * 0.2f;

        /* Step 1: Find maximum value for numerical stability */
        float max_val = s_logits[0];
        for (int i = 1; i < 16; i++) {
            if (s_logits[i] > max_val) max_val = s_logits[i];
        }

        /* Step 2: Exponential and sum */
        float sum = 0.0f;
        for (int i = 0; i < 16; i++) {
            s_probabilities[i] = expf(s_logits[i] - max_val);
            sum += s_probabilities[i];
        }

        /* Step 3: Normalize */
        float inv_sum = 1.0f / sum;
        for (int i = 0; i < 16; i++) {
            s_probabilities[i] *= inv_sum;
        }
    }
    g_sink = (uint32_t)(s_probabilities[0] * 1000.0f);
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)iterations / time_sec / 1000.0f) : 0.0f; /* kOps/s */
}
