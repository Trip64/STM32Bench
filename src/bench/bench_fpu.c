/* FPU Single-Precision & Double-Precision Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include <math.h>

extern volatile uint32_t g_sink;

/* 1. Mandelbrot Set (Single Precision 32-bit float) */
void Bench_FPU_MandelbrotSP(BenchResult *res)
{
    const int width = 64;
    const int height = 64;
    const int max_iter = 100;
    uint32_t total_iterations = 0;

    BENCH_START();
    for (int y = 0; y < height; y++) {
        float ci = -1.2f + ((float)y / (float)height) * 2.4f;
        for (int x = 0; x < width; x++) {
            float cr = -2.0f + ((float)x / (float)width) * 3.0f;
            float zr = 0.0f, zi = 0.0f;
            int iter = 0;
            while ((zr * zr + zi * zi <= 4.0f) && (iter < max_iter)) {
                float temp = zr * zr - zi * zi + cr;
                zi = 2.0f * zr * zi + ci;
                zr = temp;
                iter++;
            }
            total_iterations += iter;
        }
    }
    g_sink = total_iterations;
    BENCH_STOP();

    /* Each inner iteration involves ~8 FLOPs (mul, sub, mul, mul, add, add) */
    float total_flops = (float)total_iterations * 8.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_flops / time_sec / 1000000.0f) : 0.0f; /* MFLOPS */
}

/* 2. Mandelbrot Set (Double Precision 64-bit double) */
void Bench_FPU_MandelbrotDP(BenchResult *res)
{
    if (!PAL_HasDPFPU()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    const int width = 64;
    const int height = 64;
    const int max_iter = 100;
    uint32_t total_iterations = 0;

    BENCH_START();
    for (int y = 0; y < height; y++) {
        double ci = -1.2 + ((double)y / (double)height) * 2.4;
        for (int x = 0; x < width; x++) {
            double cr = -2.0 + ((double)x / (double)width) * 3.0;
            double zr = 0.0, zi = 0.0;
            int iter = 0;
            while ((zr * zr + zi * zi <= 4.0) && (iter < max_iter)) {
                double temp = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp;
                iter++;
            }
            total_iterations += iter;
        }
    }
    g_sink = total_iterations;
    BENCH_STOP();

    float total_flops = (float)total_iterations * 8.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_flops / time_sec / 1000000.0f) : 0.0f; /* MFLOPS */
}

#if BENCH_SUITE_EXTENDED
/* 3. Complex FFT 256-point (Single Precision) */
#define FFT_SIZE 256
static float s_fft_real[FFT_SIZE];
static float s_fft_imag[FFT_SIZE];

static void fft_radix2(float *rex, float *imx, int n)
{
    int j = n / 2;
    for (int i = 1; i < n - 1; i++) {
        if (i < j) {
            float tr = rex[j]; rex[j] = rex[i]; rex[i] = tr;
            float ti = imx[j]; imx[j] = imx[i]; imx[i] = ti;
        }
        int k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    for (int l = 1; l <= 8; l++) { /* log2(256) = 8 */
        int le = 1 << l;
        int le2 = le / 2;
        float ur = 1.0f;
        float ui = 0.0f;
        float sr = cosf((float)M_PI / (float)le2);
        float si = -sinf((float)M_PI / (float)le2);

        for (int m = 1; m <= le2; m++) {
            for (int i = m - 1; i < n; i += le) {
                int ip = i + le2;
                float tr = rex[ip] * ur - imx[ip] * ui;
                float ti = rex[ip] * ui + imx[ip] * ur;
                rex[ip] = rex[i] - tr;
                imx[ip] = imx[i] - ti;
                rex[i] += tr;
                imx[i] += ti;
            }
            float temp_ur = ur * sr - ui * si;
            ui = ur * si + ui * sr;
            ur = temp_ur;
        }
    }
}

void Bench_FPU_FFT(BenchResult *res)
{
    const int count = 200;

    for (int i = 0; i < FFT_SIZE; i++) {
        s_fft_real[i] = sinf((float)i * 0.1f);
        s_fft_imag[i] = 0.0f;
    }

    BENCH_START();
    for (int c = 0; c < count; c++) {
        fft_radix2(s_fft_real, s_fft_imag, FFT_SIZE);
    }
    g_sink = (uint32_t)s_fft_real[0];
    BENCH_STOP();

    float total_ffts = (float)count;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_ffts / time_sec / 1000.0f) : 0.0f; /* kFFT/s */
}
#else
void Bench_FPU_FFT(BenchResult *res)
{
    res->available = false;
    res->score = 0.0f;
}
#endif

/* 4. Matrix Multiplication 4x4 (SP float) */
void Bench_FPU_MatrixMulSP(BenchResult *res)
{
    const int runs = 50000;
    float A[4][4] = {{1.1f, 2.2f, 3.3f, 4.4f}, {5.5f, 6.6f, 7.7f, 8.8f}, {9.9f, 0.1f, 1.2f, 2.3f}, {3.4f, 4.5f, 5.6f, 6.7f}};
    float B[4][4] = {{0.9f, 1.8f, 2.7f, 3.6f}, {4.5f, 5.4f, 6.3f, 7.2f}, {8.1f, 9.0f, 0.2f, 1.3f}, {2.4f, 3.5f, 4.6f, 5.7f}};
    float C[4][4] = {0};

    BENCH_START();
    for (int r = 0; r < runs; r++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
        A[0][0] = C[3][3] * 0.0001f + 1.1f; /* Data dependency to prevent dead code removal */
    }
    g_sink = (uint32_t)C[0][0];
    BENCH_STOP();

    /* 4x4x4 muls + 4x4x3 adds = 64 + 48 = 112 FLOPs per matrix mul */
    float total_flops = (float)runs * 112.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_flops / time_sec / 1000000.0f) : 0.0f; /* MFLOPS */
}

/* 5. Matrix Multiplication 4x4 (DP double) */
void Bench_FPU_MatrixMulDP(BenchResult *res)
{
    if (!PAL_HasDPFPU()) {
        res->available = false;
        res->score = 0.0f;
        return;
    }

    const int runs = 50000;
    double A[4][4] = {{1.1, 2.2, 3.3, 4.4}, {5.5, 6.6, 7.7, 8.8}, {9.9, 0.1, 1.2, 2.3}, {3.4, 4.5, 5.6, 6.7}};
    double B[4][4] = {{0.9, 1.8, 2.7, 3.6}, {4.5, 5.4, 6.3, 7.2}, {8.1, 9.0, 0.2, 1.3}, {2.4, 3.5, 4.6, 5.7}};
    double C[4][4] = {0};

    BENCH_START();
    for (int r = 0; r < runs; r++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                double sum = 0.0;
                for (int k = 0; k < 4; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
        A[0][0] = C[3][3] * 0.0001 + 1.1;
    }
    g_sink = (uint32_t)C[0][0];
    BENCH_STOP();

    float total_flops = (float)runs * 112.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_flops / time_sec / 1000000.0f) : 0.0f; /* MFLOPS */
}
