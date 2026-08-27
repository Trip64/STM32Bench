/* Core benchmark runner, result manager, and comprehensive STM32Mark scoring engine */

#include "bench_engine.h"
#include "pal.h"
#include <string.h>

/* Forward declarations: CPU */
void Bench_CPU_Dhrystone(BenchResult *res);
void Bench_CPU_Sieve(BenchResult *res);
void Bench_CPU_Sort(BenchResult *res);
void Bench_CPU_CRC32(BenchResult *res);
void Bench_CPU_IPC(BenchResult *res);
void Bench_CPU_BitOps(BenchResult *res);
void Bench_CPU_Branch(BenchResult *res);
void Bench_CPU_SHA256(BenchResult *res);

/* Forward declarations: FPU */
void Bench_FPU_MandelbrotSP(BenchResult *res);
void Bench_FPU_MandelbrotDP(BenchResult *res);
void Bench_FPU_FFT(BenchResult *res);
void Bench_FPU_MatrixMulSP(BenchResult *res);
void Bench_FPU_MatrixMulDP(BenchResult *res);

/* Forward declarations: DSP & Coprocessors */
void Bench_DSP_DotProduct(BenchResult *res);
void Bench_DSP_FIR(BenchResult *res);
void Bench_DSP_VectorMag(BenchResult *res);
void Bench_CORDIC_SinCos(BenchResult *res);
void Bench_CORDIC_Atan2(BenchResult *res);
void Bench_CORDIC_Sqrt(BenchResult *res);
void Bench_FMAC_FIR(BenchResult *res);

/* Forward declarations: 2D & 3D Graphics */
void Bench_2D_Fill(BenchResult *res);
void Bench_2D_AlphaBlend(BenchResult *res);
void Bench_2D_Bresenham(BenchResult *res);
void Bench_3D_Transform(BenchResult *res);
void Bench_3D_Rasterize(BenchResult *res);
void Bench_3D_Raymarch(BenchResult *res);

/* Forward declarations: TinyML & Edge AI */
void Bench_ML_Conv2D(BenchResult *res);
void Bench_ML_Dense(BenchResult *res);
void Bench_ML_Softmax(BenchResult *res);

/* Forward declarations: Hardware RNG & Cryptography */
void Bench_Crypto_RNG(BenchResult *res);
void Bench_Crypto_AES(BenchResult *res);
void Bench_Crypto_ChaCha20(BenchResult *res);

/* Forward declarations: Data Compression */
void Bench_Comp_LZ4(BenchResult *res);
void Bench_Comp_RLE(BenchResult *res);

/* Forward declarations: Audio DSP */
void Bench_Audio_RFFT(BenchResult *res);
void Bench_Audio_Biquad(BenchResult *res);

/* Forward declarations: Real-Time OS & Interrupts */
void Bench_RT_IRQLatency(BenchResult *res);
void Bench_RT_ContextSwitch(BenchResult *res);
void Bench_RT_Atomic(BenchResult *res);

/* Forward declarations: GPIO, Bus & DMA */
void Bench_IO_GPIOToggle(BenchResult *res);
void Bench_IO_GPIORead(BenchResult *res);
void Bench_IO_DMAM2M(BenchResult *res);

/* Forward declarations: Memory Subsystem */
void Bench_Mem_Memcpy(BenchResult *res);
void Bench_Mem_Memset(BenchResult *res);
void Bench_Mem_DTCMvsFlash(BenchResult *res);
void Bench_Mem_CacheImpact(BenchResult *res);

/* Benchmark Registry: Tiered Basic (Universal) + Extended Suites */
static const BenchDefinition s_bench_defs[] = {
    /* ── 1. Core Universal Benchmarks (Run on ANY MCU: M0 to M7, <6KB RAM) ─ */
    { "cpu_dhry",    "Integer ALU Throughput",  "CPU", "Arithmetic, logic shifts, branches (10k ops)", "MIPS",       Bench_CPU_Dhrystone },
    { "cpu_sieve",   "Sieve of Eratosthenes",   "CPU", "Primes up to 10k, memory & bit operations",    "kOps/s",     Bench_CPU_Sieve },
    { "cpu_sort",    "QuickSort (256/1024 ints)","CPU","Array sorting, branch prediction & reads",     "kOps/s",     Bench_CPU_Sort },
    { "cpu_crc32",   "Software CRC32",          "CPU", "Bit manipulation throughput on block data",    "MB/s",       Bench_CPU_CRC32 },
    { "cpu_bitops",  "Bitfield Operations",     "CPU", "Bit reversal, CLZ, endian byte swap",         "MOps/s",     Bench_CPU_BitOps },
    { "cpu_branch",  "Branch Predictor Stress", "CPU", "Data-dependent alternating branching penalty",  "kPasses/s",  Bench_CPU_Branch },
    { "fpu_mandel_sp","Mandelbrot (SP Float)",  "FPU", "Single-Precision 32-bit fractal loop",         "MFLOPS",     Bench_FPU_MandelbrotSP },
    { "fpu_matmul_sp","Matrix Multiply (SP)",   "FPU", "4x4 single precision matrix mul",              "MFLOPS",     Bench_FPU_MatrixMulSP },
    { "dsp_dotprod", "Vector Dot Product (Q15)","DSP", "Dual-MAC SIMD or portable scalar Q15 loop",   "MMAC/s",     Bench_DSP_DotProduct },
    { "dsp_fir",     "FIR Filter (Q15)",        "DSP", "Cascaded FIR filter processing",               "MSamples/s", Bench_DSP_FIR },
    { "io_gpio_bsrr", "GPIO Pin Toggle (BSRR)",  "IO",  "Atomic pin set/reset toggle speed",           "MTogg/s",    Bench_IO_GPIOToggle },
    { "io_gpio_read", "GPIO Input Read (IDR)",   "IO",  "Continuous port read bandwidth",              "MReads/s",   Bench_IO_GPIORead },
    { "mem_memcpy",   "Memcpy Bandwidth",        "Memory","32-bit aligned block memory copy",           "MB/s",       Bench_Mem_Memcpy },
    { "mem_memset",   "Memset Bandwidth",        "Memory","Fast memory fill write throughput",          "MB/s",       Bench_Mem_Memset },

#if BENCH_SUITE_EXTENDED
    /* ── 2. Extended CPU Architecture & Cryptography ──────────────────── */
    { "cpu_ipc",     "Dual-Issue Superscalar",  "CPU", "Cortex-M7 dual-ALU instruction-level parallel", "IPC",        Bench_CPU_IPC },
    { "cpu_sha256",  "Cryptographic SHA-256",   "CPU", "SHA-256 block hashing throughput (64KB)",      "MB/s",       Bench_CPU_SHA256 },
    { "cpu_hw_crc",  "Hardware CRC-32 Engine",  "CPU", "Dedicated on-chip hardware CRC unit (AHB4)",   "MB/s",       Bench_CPU_HwCRC },

    /* ── 3. Extended Floating Point & DP-FPU ──────────────────────────── */
    { "fpu_mandel_dp","Mandelbrot (DP Double)", "FPU", "Double-Precision DP-FPU 64-bit fractal loop",  "MFLOPS",     Bench_FPU_MandelbrotDP },
    { "fpu_fft",      "Complex FFT 256-pt",     "FPU", "Radix-2 single-precision complex FFT",         "kFFT/s",     Bench_FPU_FFT },
    { "fpu_matmul_dp","Matrix Multiply (DP)",   "FPU", "4x4 double precision matrix mul (5000x)",      "MFLOPS",     Bench_FPU_MatrixMulDP },

    /* ── 4. Extended DSP & Silicon Coprocessors ────────────────────────── */
    { "dsp_mag",     "Vector Magnitude (Q15)",  "DSP", "Fixed-point vector complex magnitude",         "kVec/s",     Bench_DSP_VectorMag },
    { "cordic_sincos","CORDIC Sin/Cos vs Soft", "CORDIC","Hardware CORDIC angle math vs sinf/cosf",    "x Speedup",  Bench_CORDIC_SinCos },
    { "cordic_atan2", "CORDIC Atan2 vs Soft",   "CORDIC","Hardware CORDIC arctangent vs atan2f",       "x Speedup",  Bench_CORDIC_Atan2 },
    { "cordic_sqrt",  "CORDIC Sqrt vs Soft",    "CORDIC","Hardware CORDIC square root vs sqrtf",       "x Speedup",  Bench_CORDIC_Sqrt },
    { "fmac_fir",     "FMAC Hardware FIR",      "FMAC",  "Dedicated hardware filter coprocessor vs CPU","x Speedup",  Bench_FMAC_FIR },

    /* ── 5. 2D & 3D Graphics Acceleration ─────────────────────────────── */
    { "gfx_2d_fill",  "2D Rect Fill (DMA2D)",   "GFX", "Chrom-ART DMA2D solid color fill (ARGB8888)",  "MPixels/s",  Bench_2D_Fill },
    { "gfx_2d_blend", "2D Alpha Blend (DMA2D)", "GFX", "DMA2D pixel blending & format conversion",     "MPixels/s",  Bench_2D_AlphaBlend },
    { "gfx_2d_line",  "Bresenham 2D Line Draw", "GFX", "Integer geometric line rasterization",         "kLines/s",   Bench_2D_Bresenham },
    { "gfx_3d_trans", "3D MVP Vertex Transform","GFX", "3D vertex rotation, 4x4 matrix projection",    "kVerts/s",   Bench_3D_Transform },
    { "gfx_3d_raster","3D Triangle Rasterizer", "GFX", "Barycentric coordinate rasterizer & depth",    "kTris/s",    Bench_3D_Rasterize },
    { "gfx_3d_ray",   "3D SDF Raymarching",     "GFX", "Sphere & torus signed distance field raymarch", "kRays/s",    Bench_3D_Raymarch },

    /* ── 6. TinyML & Edge AI Inference ────────────────────────────────── */
    { "ai_conv2d",    "Quantized Conv2D (int8)", "AI", "2D Convolution layer (16x16, 3x3k, 8ch, ReLU)","MMAC/s",     Bench_ML_Conv2D },
    { "ai_dense",     "Dense Layer (64->16)",    "AI", "Quantized fully-connected inference layer",     "kInf/s",     Bench_ML_Dense },
    { "ai_softmax",   "Softmax Activation",      "AI", "Vectorized 16-class numerical softmax",        "kOps/s",     Bench_ML_Softmax },

    /* ── 7. Hardware Security & Cryptography ──────────────────────────── */
    { "crypto_rng",   "Hardware True RNG",       "Crypto","STM32 on-chip physical entropy generator",   "MB/s",       Bench_Crypto_RNG },
    { "crypto_aes",   "AES-128 Block Cipher",    "Crypto","Symmetric 128-bit block encryption",         "MB/s",       Bench_Crypto_AES },
    { "crypto_chacha","ChaCha20 Stream Cipher",  "Crypto","256-bit stream cipher block encryption",     "MB/s",       Bench_Crypto_ChaCha20 },

    /* ── 8. Data Compression ──────────────────────────────────────────── */
    { "comp_lz4",     "LZ4 Decompression",       "Compress","Streaming LZ4 token/offset byte decode",   "MB/s",       Bench_Comp_LZ4 },
    { "comp_rle",     "RLE Byte-Pack Compress",  "Compress","Run-length encoding on telemetry stream",  "MB/s",       Bench_Comp_RLE },

    /* ── 9. Audio & DSP Spectrum ──────────────────────────────────────── */
    { "audio_rfft",   "Real FFT 512-pt Window",  "Audio", "512-pt real FFT with Hanning window",        "kFFT/s",     Bench_Audio_RFFT },
    { "audio_biquad", "8-Stage Biquad IIR EQ",   "Audio", "Direct Form II Transposed parametric audio", "MSamples/s", Bench_Audio_Biquad },

    /* ── 10. Real-Time OS & Interrupt Latency ─────────────────────────── */
    { "rt_irqlat",    "NVIC Interrupt Latency",  "RealTime","Hardware cycle delay from trigger to ISR", "Cycles",     Bench_RT_IRQLatency },
    { "rt_ctxsw",     "RTOS Context Switch",     "RealTime","Full Cortex-M callee + FPU frame switch",  "kSwitches/s",Bench_RT_ContextSwitch },
    { "rt_atomic",    "Atomic Exclusive Monitor","RealTime","LDREX/STREX spinlock sync primitives",    "MOps/s",     Bench_RT_Atomic },

    /* ── 11. Bus Masters & DMA ────────────────────────────────────────── */
    { "io_dma_m2m",   "DMA Memory-to-Memory",    "IO",    "DMA 32-bit hardware block copy",             "MB/s",       Bench_IO_DMAM2M },
    { "io_mdma",      "64-bit Master DMA (AXI)", "IO",    "64-bit AXI bus master DMA transfer",         "MB/s",       Bench_IO_MDMA },

    /* ── 12. Memory Hierarchy, Caches & TCM ───────────────────────────── */
    { "mem_latency",  "DTCM vs Flash Latency",   "Memory","0-wait state DTCM vs Flash wait states",     "x Ratio",    Bench_Mem_DTCMvsFlash },
    { "mem_cache",    "D-Cache Hit/Miss Impact", "Memory","Execution speedup with D-Cache ON vs OFF",   "x Speedup",  Bench_Mem_CacheImpact },
    { "mem_itcm",     "ITCM 0-Wait Execution",   "Memory","0-wait state ITCM instruction execution",    "x Ratio",    Bench_Mem_ITCM },
    { "mem_cache_bench","L1 D-Cache Line Stride","Memory","32-byte cache line burst fill throughput",   "MB/s",       Bench_Mem_CacheStride },
#endif
};

#define BENCH_DEF_COUNT (sizeof(s_bench_defs) / sizeof(s_bench_defs[0]))

static BenchResult s_results[BENCH_DEF_COUNT];
static bool s_initialized = false;

void Bench_Init(void)
{
    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        s_results[i].id          = s_bench_defs[i].id;
        s_results[i].name        = s_bench_defs[i].name;
        s_results[i].category    = s_bench_defs[i].category;
        s_results[i].description = s_bench_defs[i].description;
        s_results[i].unit        = s_bench_defs[i].unit;
        s_results[i].cycles      = 0;
        s_results[i].time_us     = 0;
        s_results[i].score       = 0.0f;
        s_results[i].available   = true;
    }
    s_initialized = true;
}

static void bench_set_category_leds(const char *category, size_t index)
{
    if (strcmp(category, "CPU") == 0) {
        PAL_LED_Set(0, true); PAL_LED_Set(1, (index & 1) != 0); PAL_LED_Set(2, false);
    } else if (strcmp(category, "FPU") == 0) {
        PAL_LED_Set(0, (index & 1) != 0); PAL_LED_Set(1, true); PAL_LED_Set(2, false);
    } else if (strcmp(category, "DSP") == 0 || strcmp(category, "Audio") == 0) {
        PAL_LED_Set(0, true); PAL_LED_Set(1, true); PAL_LED_Set(2, (index & 1) != 0);
    } else if (strcmp(category, "CORDIC") == 0 || strcmp(category, "FMAC") == 0) {
        PAL_LED_Set(0, false); PAL_LED_Set(1, (index & 1) != 0); PAL_LED_Set(2, true);
    } else if (strcmp(category, "GFX") == 0) {
        PAL_LED_Set(0, (index & 1) == 0); PAL_LED_Set(1, true); PAL_LED_Set(2, (index & 1) != 0);
    } else if (strcmp(category, "AI") == 0) {
        /* TinyML: Green + Yellow active */
        PAL_LED_Set(0, true); PAL_LED_Set(1, true); PAL_LED_Set(2, false);
    } else if (strcmp(category, "Crypto") == 0) {
        /* Security / RNG: Red active */
        PAL_LED_Set(0, false); PAL_LED_Set(1, false); PAL_LED_Set(2, true);
    } else if (strcmp(category, "IO") == 0 || strcmp(category, "RealTime") == 0) {
        /* High speed IO / ISR: Alternating sweep */
        PAL_LED_Set(0, (index & 1) == 0); PAL_LED_Set(1, (index & 1) != 0); PAL_LED_Set(2, true);
    } else {
        PAL_LED_Set(0, (index % 3) == 0); PAL_LED_Set(1, (index % 3) == 1); PAL_LED_Set(2, (index % 3) == 2);
    }
}

static void bench_completion_led_sweep(void)
{
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int led = 0; led < 3; led++) {
            PAL_LED_Set(0, led == 0);
            PAL_LED_Set(1, led == 1);
            PAL_LED_Set(2, led == 2);
            PAL_DelayMs(25);
        }
    }
    PAL_LED_Set(0, true);
    PAL_LED_Set(1, false);
    PAL_LED_Set(2, false);
}

void Bench_RunSingle(const char *id)
{
    if (!s_initialized) Bench_Init();

    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        if (strcmp(s_bench_defs[i].id, id) == 0) {
            bench_set_category_leds(s_bench_defs[i].category, i);
            s_bench_defs[i].run(&s_results[i]);
            bench_completion_led_sweep();
            break;
        }
    }
}

void Bench_RunCategory(const char *category)
{
    if (!s_initialized) Bench_Init();

    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        if (category == NULL || strcmp(s_bench_defs[i].category, category) == 0) {
            bench_set_category_leds(s_bench_defs[i].category, i);
            s_bench_defs[i].run(&s_results[i]);
        }
    }
    bench_completion_led_sweep();
}

void Bench_RunAll(void)
{
    if (!s_initialized) Bench_Init();

    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        bench_set_category_leds(s_bench_defs[i].category, i);
        s_bench_defs[i].run(&s_results[i]);
    }
    bench_completion_led_sweep();
}

size_t Bench_GetCount(void)
{
    return BENCH_DEF_COUNT;
}

const BenchResult* Bench_GetResult(size_t index)
{
    if (index >= BENCH_DEF_COUNT) return NULL;
    return &s_results[index];
}

const BenchResult* Bench_FindResult(const char *id)
{
    if (!id) return NULL;
    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        if (strcmp(s_results[i].id, id) == 0) {
            return &s_results[i];
        }
    }
    return NULL;
}

/* ── COMPREHENSIVE 8-CATEGORY STM32MARK SCORING ENGINE ──────────────── */

/* ── SCIENTIFIC GEOMETRIC MEAN BASELINE SCORING ENGINE ─────────────── */
/* Reference: Standard ARM Cortex-M4 @ 100 MHz scores 1,000 points everywhere.
 * A score of 15,000 means exactly 15.0x faster than the reference baseline.
 */
#include <math.h>

static float get_benchmark_normalized_points(const BenchResult *r)
{
    if (r->score <= 0.0f) return 0.0f;

    /* Base rates calibrated so 1,000 pts = baseline microcontroller execution speed (1.0 IPC @ 100MHz).
     * At 550MHz with dual-issue superscalar + caches, expected scores are ~10,000 - 18,000 pts.
     */
    float base = 1.0f;

    /* CPU */
    if (strcmp(r->id, "cpu_dhry") == 0)            base = 25.0f;    /* MIPS */
    else if (strcmp(r->id, "cpu_sieve") == 0)       base = 0.8f;     /* kOps/s */
    else if (strcmp(r->id, "cpu_sort") == 0)        base = 0.4f;     /* kOps/s */
    else if (strcmp(r->id, "cpu_crc32") == 0)       base = 1.2f;     /* MB/s */
    else if (strcmp(r->id, "cpu_ipc") == 0)         base = 0.18f;    /* IPC (2.0 IPC = ~11,000 pts) */
    else if (strcmp(r->id, "cpu_bitops") == 0)      base = 15.0f;    /* MOps/s */
    else if (strcmp(r->id, "cpu_branch") == 0)      base = 220.0f;   /* kPasses/s */
    else if (strcmp(r->id, "cpu_sha256") == 0)      base = 1.6f;     /* MB/s */

    /* FPU (Single & Double Precision) */
    else if (strcmp(r->id, "fpu_mandel_sp") == 0)   base = 12.0f;    /* MFLOPS */
    else if (strcmp(r->id, "fpu_mandel_dp") == 0)   base = 6.5f;     /* MFLOPS */
    else if (strcmp(r->id, "fpu_fft") == 0)         base = 1.2f;     /* kFFT/s */
    else if (strcmp(r->id, "fpu_matmul_sp") == 0)   base = 3200.0f;  /* MFLOPS */
    else if (strcmp(r->id, "fpu_matmul_dp") == 0)   base = 2200.0f;  /* MFLOPS */

    /* DSP, Audio & Coprocessors */
    else if (strcmp(r->id, "dsp_dotprod") == 0)     base = 22.0f;    /* MMAC/s */
    else if (strcmp(r->id, "dsp_fir") == 0)         base = 0.6f;     /* MSamples/s */
    else if (strcmp(r->id, "dsp_mag") == 0)         base = 1.5f;     /* kVec/s */
    else if (strcmp(r->id, "cordic_sincos") == 0)   return (r->score > 1.0f ? r->score : 1.0f) * 2500.0f;
    else if (strcmp(r->id, "cordic_atan2") == 0)    return (r->score > 1.0f ? r->score : 1.0f) * 2500.0f;
    else if (strcmp(r->id, "cordic_sqrt") == 0)     return (r->score > 1.0f ? r->score : 1.0f) * 2500.0f;
    else if (strcmp(r->id, "fmac_fir") == 0)        return (r->score > 1.0f ? r->score : 1.0f) * 4000.0f;
    else if (strcmp(r->id, "audio_rfft") == 0)      base = 0.5f;     /* kFFT/s */
    else if (strcmp(r->id, "audio_biquad") == 0)    base = 0.25f;    /* MSamples/s */

    /* 2D/3D GFX */
    else if (strcmp(r->id, "gfx_2d_fill") == 0)     base = 22.0f;    /* MPixels/s */
    else if (strcmp(r->id, "gfx_2d_blend") == 0)    base = 12.0f;    /* MPixels/s */
    else if (strcmp(r->id, "gfx_2d_line") == 0)     base = 95.0f;    /* kLines/s */
    else if (strcmp(r->id, "gfx_3d_trans") == 0)    base = 450.0f;   /* kVerts/s */
    else if (strcmp(r->id, "gfx_3d_raster") == 0)   base = 1100.0f;  /* kTris/s */
    else if (strcmp(r->id, "gfx_3d_ray") == 0)      base = 50.0f;    /* kRays/s */

    /* TinyML */
    else if (strcmp(r->id, "ai_conv2d") == 0)       base = 14.0f;    /* MMAC/s */
    else if (strcmp(r->id, "ai_dense") == 0)        base = 12.0f;    /* kInf/s */
    else if (strcmp(r->id, "ai_softmax") == 0)      base = 24.0f;    /* kOps/s */

    /* Crypto & Compression */
    else if (strcmp(r->id, "crypto_rng") == 0)      base = 1.2f;     /* MB/s */
    else if (strcmp(r->id, "crypto_aes") == 0)      base = 180.0f;   /* MB/s */
    else if (strcmp(r->id, "crypto_chacha") == 0)   base = 15.0f;    /* MB/s */
    else if (strcmp(r->id, "comp_lz4") == 0)        base = 9.0f;     /* MB/s */
    else if (strcmp(r->id, "comp_rle") == 0)        base = 6.0f;     /* MB/s */

    /* IO & Real-Time */
    else if (strcmp(r->id, "io_gpio_bsrr") == 0)    base = 4.0f;     /* MTogg/s */
    else if (strcmp(r->id, "io_gpio_read") == 0)    base = 3.5f;     /* MReads/s */
    else if (strcmp(r->id, "io_dma_m2m") == 0)      base = 22.0f;    /* MB/s */
    else if (strcmp(r->id, "rt_irqlat") == 0)       return (r->score > 0 ? (350.0f / r->score) : 14.0f) * 1000.0f;
    else if (strcmp(r->id, "rt_ctxsw") == 0)        base = 2000.0f;  /* kSwitches/s */
    else if (strcmp(r->id, "rt_atomic") == 0)       base = 2.4f;     /* MOps/s */

    /* Memory */
    else if (strcmp(r->id, "mem_memcpy") == 0)      base = 15.0f;    /* MB/s */
    else if (strcmp(r->id, "mem_memset") == 0)      base = 22.0f;    /* MB/s */
    else if (strcmp(r->id, "mem_latency") == 0)     return (r->score > 1.0f ? r->score : 1.0f) * 10000.0f;
    else if (strcmp(r->id, "mem_cache") == 0)       return (r->score > 1.0f ? r->score : 1.0f) * 4500.0f;
    else if (strcmp(r->id, "mem_itcm") == 0)        return (r->score > 1.0f ? r->score : 1.0f) * 10000.0f;
    else if (strcmp(r->id, "mem_cache_bench") == 0) base = 25.0f;    /* MB/s */
    else if (strcmp(r->id, "cpu_hw_crc") == 0)      base = 50.0f;    /* MB/s */
    else if (strcmp(r->id, "io_mdma") == 0)         base = 80.0f;    /* MB/s */

    return (r->score / base) * 1000.0f;
}

const char* Bench_GetSuiteName(void)
{
#if BENCH_SUITE_EXTENDED
    return "EXTENDED";
#else
    return "BASIC";
#endif
}

static inline uint32_t calc_geom_mean(float log_sum, int count)
{
    if (count <= 0) return 0;
    return (uint32_t)expf(log_sum / (float)count);
}

BenchCategoryScores Bench_GetScores(void)
{
    BenchCategoryScores s = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    float log_cpu = 0.0f; int cnt_cpu = 0;
    float log_fpu = 0.0f; int cnt_fpu = 0;
    float log_dsp = 0.0f; int cnt_dsp = 0;
    float log_gfx = 0.0f; int cnt_gfx = 0;
    float log_ai  = 0.0f; int cnt_ai  = 0;
    float log_cry = 0.0f; int cnt_cry = 0;
    float log_io  = 0.0f; int cnt_io  = 0;
    float log_mem = 0.0f; int cnt_mem = 0;

    for (size_t i = 0; i < BENCH_DEF_COUNT; i++) {
        const BenchResult *r = &s_results[i];
        float pts = get_benchmark_normalized_points(r);
        if (pts <= 0.0f) continue;
        float log_pts = logf(pts);

        if (strcmp(r->category, "CPU") == 0) {
            log_cpu += log_pts; cnt_cpu++;
        } else if (strcmp(r->category, "FPU") == 0) {
            log_fpu += log_pts; cnt_fpu++;
        } else if (strcmp(r->category, "DSP") == 0 || strcmp(r->category, "CORDIC") == 0 ||
                   strcmp(r->category, "FMAC") == 0 || strcmp(r->category, "Audio") == 0) {
            log_dsp += log_pts; cnt_dsp++;
        } else if (strcmp(r->category, "GFX") == 0) {
            log_gfx += log_pts; cnt_gfx++;
        } else if (strcmp(r->category, "AI") == 0) {
            log_ai += log_pts; cnt_ai++;
        } else if (strcmp(r->category, "Crypto") == 0 || strcmp(r->category, "Compress") == 0) {
            log_cry += log_pts; cnt_cry++;
        } else if (strcmp(r->category, "IO") == 0 || strcmp(r->category, "RealTime") == 0) {
            log_io += log_pts; cnt_io++;
        } else if (strcmp(r->category, "Memory") == 0) {
            log_mem += log_pts; cnt_mem++;
        }
    }

    s.cpu    = calc_geom_mean(log_cpu, cnt_cpu);
    s.fpu    = calc_geom_mean(log_fpu, cnt_fpu);
    s.dsp    = calc_geom_mean(log_dsp, cnt_dsp);
    s.gfx    = calc_geom_mean(log_gfx, cnt_gfx);
    s.ai     = calc_geom_mean(log_ai, cnt_ai);
    s.crypto = calc_geom_mean(log_cry, cnt_cry);
    s.io     = calc_geom_mean(log_io, cnt_io);
    s.mem    = calc_geom_mean(log_mem, cnt_mem);

    /* Balanced Geometric Mean of all active category marks */
    float cat_log_sum = 0.0f;
    int cat_count = 0;
    if (s.cpu)    { cat_log_sum += logf((float)s.cpu);    cat_count++; }
    if (s.fpu)    { cat_log_sum += logf((float)s.fpu);    cat_count++; }
    if (s.dsp)    { cat_log_sum += logf((float)s.dsp);    cat_count++; }
    if (s.gfx)    { cat_log_sum += logf((float)s.gfx);    cat_count++; }
    if (s.ai)     { cat_log_sum += logf((float)s.ai);     cat_count++; }
    if (s.crypto) { cat_log_sum += logf((float)s.crypto); cat_count++; }
    if (s.io)     { cat_log_sum += logf((float)s.io);     cat_count++; }
    if (s.mem)    { cat_log_sum += logf((float)s.mem);    cat_count++; }

    s.total = calc_geom_mean(cat_log_sum, cat_count);
    return s;
}

uint32_t Bench_GetTotalScore(void)
{
    BenchCategoryScores s = Bench_GetScores();
    return s.total;
}
