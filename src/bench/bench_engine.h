/* Core Benchmark Engine for STM32Benchmark suite */

#ifndef BENCH_ENGINE_H
#define BENCH_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_BENCH_RESULTS 64

typedef struct {
    const char *id;          /* Short identifier, e.g. "mandel_sp" */
    const char *name;        /* Human readable, e.g. "Mandelbrot (Single Precision)" */
    const char *category;    /* "CPU", "FPU", "DSP", "CORDIC", "FMAC", "GFX", "AI", "Crypto", "IO", "Memory" */
    const char *description; /* Details of what this measures */
    uint32_t    cycles;      /* Measured execution cycles */
    uint32_t    time_us;     /* Microseconds */
    float       score;       /* Main performance score */
    const char *unit;        /* "MIPS", "MFLOPS", "MB/s", "kOps/s", "x Speedup" */
    bool        available;   /* True if hardware feature present and test ran */
} BenchResult;

typedef void (*BenchRunFunc)(BenchResult *res);

typedef struct {
    const char   *id;
    const char   *name;
    const char   *category;
    const char   *description;
    const char   *unit;
    BenchRunFunc  run;
} BenchDefinition;

#ifdef __cplusplus
extern "C" {
#endif

void Bench_Init(void);
void Bench_RunAll(void);
void Bench_RunCategory(const char *category);
void Bench_RunSingle(const char *id);

size_t Bench_GetCount(void);
const BenchResult* Bench_GetResult(size_t index);
const BenchResult* Bench_FindResult(const char *id);

typedef struct {
    uint32_t total;  /* Overall composite STM32Mark */
    uint32_t cpu;    /* CPU Integer mark */
    uint32_t fpu;    /* FPU Float mark */
    uint32_t dsp;    /* DSP, Audio, CORDIC & FMAC mark */
    uint32_t gfx;    /* 2D & 3D Graphics mark */
    uint32_t ai;     /* TinyML & Edge AI inference mark */
    uint32_t crypto; /* Hardware RNG & Cryptography mark */
    uint32_t io;     /* GPIO, NVIC Latency & DMA mark */
    uint32_t mem;    /* Memory bandwidth & latency mark */
} BenchCategoryScores;

#ifndef BENCH_SUITE_EXTENDED
  #if defined(STM32H723xx) || defined(STM32H7) || defined(STM32F4) || defined(STM32G4)
    #define BENCH_SUITE_EXTENDED 1
  #else
    #define BENCH_SUITE_EXTENDED 0
  #endif
#endif

BenchCategoryScores Bench_GetScores(void);
uint32_t            Bench_GetTotalScore(void);
const char*         Bench_GetSuiteName(void);

/* Advanced H7 & Extended Benchmarks */
#if BENCH_SUITE_EXTENDED
void Bench_Mem_ITCM(BenchResult *res);
void Bench_Mem_CacheStride(BenchResult *res);
void Bench_CPU_HwCRC(BenchResult *res);
void Bench_IO_MDMA(BenchResult *res);
#endif

/* Cycle measurement helpers */
#define BENCH_START() uint32_t _b_start = PAL_GetCycleCount()
#define BENCH_STOP()  uint32_t _b_end = PAL_GetCycleCount(); \
                      res->cycles = (_b_end >= _b_start) ? (_b_end - _b_start) : (0xFFFFFFFFUL - _b_start + _b_end); \
                      res->time_us = (uint32_t)(((uint64_t)res->cycles * 1000000ULL) / PAL_GetCoreClockHz())

#ifdef __cplusplus
}
#endif

#endif /* BENCH_ENGINE_H */
