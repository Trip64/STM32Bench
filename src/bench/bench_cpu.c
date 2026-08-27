/* CPU Integer & Core Benchmark Implementations */

#include "bench_engine.h"
#include "pal.h"
#include <stdint.h>
#include <stdbool.h>

/* Volatile sink to prevent aggressive compiler loop optimization */
volatile uint32_t g_sink = 0;

/* 1. Dhrystone-like integer ALU benchmark */
void Bench_CPU_Dhrystone(BenchResult *res)
{
    const uint32_t iterations = 50000;
    uint32_t a = 1234567, b = 7654321, c = 0;

    BENCH_START();
    for (uint32_t i = 0; i < iterations; i++) {
        a = (a ^ (b >> 3)) + 0x9E3779B9;
        b = (b << 2) ^ (a + i);
        c += (a * 33) ^ (b / (i + 1));
    }
    g_sink = a + b + c;
    BENCH_STOP();

    /* 1 loop has approx 10 basic integer operations */
    float total_ops = (float)iterations * 10.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_ops / time_sec / 1000000.0f) : 0.0f; /* MIPS */
}

/* 2. Sieve of Eratosthenes */
#if BENCH_SUITE_EXTENDED
#define SIEVE_SIZE 10000
#else
#define SIEVE_SIZE 1000
#endif
static uint8_t s_primes[SIEVE_SIZE];

void Bench_CPU_Sieve(BenchResult *res)
{
    const uint32_t passes = (BENCH_SUITE_EXTENDED) ? 50 : 200;

    BENCH_START();
    for (uint32_t p = 0; p < passes; p++) {
        for (uint32_t i = 0; i < SIEVE_SIZE; i++) s_primes[i] = 1;
        s_primes[0] = s_primes[1] = 0;

        for (uint32_t i = 2; i * i < SIEVE_SIZE; i++) {
            if (s_primes[i]) {
                for (uint32_t j = i * i; j < SIEVE_SIZE; j += i) {
                    s_primes[j] = 0;
                }
            }
        }
    }
    g_sink = s_primes[SIEVE_SIZE - 1];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)passes / time_sec / 1000.0f) : 0.0f; /* kPasses/sec */
}

/* 3. QuickSort Benchmark */
#if BENCH_SUITE_EXTENDED
#define SORT_SIZE 1024
#else
#define SORT_SIZE 256
#endif
static uint32_t s_sort_arr[SORT_SIZE];

static void qsort_bench(uint32_t *arr, int left, int right)
{
    if (left >= right) return;
    uint32_t pivot = arr[(left + right) / 2];
    int i = left, j = right;
    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) {
            uint32_t tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
            i++; j--;
        }
    }
    qsort_bench(arr, left, j);
    qsort_bench(arr, i, right);
}

void Bench_CPU_Sort(BenchResult *res)
{
    const uint32_t runs = 100;

    BENCH_START();
    for (uint32_t r = 0; r < runs; r++) {
        /* Pseudo-random LCG fill */
        uint32_t seed = r * 1337 + 7;
        for (int i = 0; i < SORT_SIZE; i++) {
            seed = seed * 1664525 + 1013904223;
            s_sort_arr[i] = seed;
        }
        qsort_bench(s_sort_arr, 0, SORT_SIZE - 1);
    }
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)runs / time_sec / 1000.0f) : 0.0f; /* kSorts/sec */
}

/* 4. Software CRC32 Benchmark (64KB buffer) */
#define CRC_BUF_SIZE (64 * 1024)
static uint8_t s_crc_buffer[1024]; /* 1KB chunk looped 64 times to save RAM */

static uint32_t crc32_soft(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

void Bench_CPU_CRC32(BenchResult *res)
{
    for (int i = 0; i < 1024; i++) s_crc_buffer[i] = (uint8_t)(i * 37);

    BENCH_START();
    uint32_t dummy = 0;
    for (int chunk = 0; chunk < 64; chunk++) {
        dummy += crc32_soft(s_crc_buffer, 1024);
    }
    g_sink = dummy;
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    float data_mb = (64.0f * 1024.0f) / (1024.0f * 1024.0f);
    res->score = (time_sec > 0) ? (data_mb / time_sec) : 0.0f; /* MB/s */
}

/* 5. Dual-Issue Superscalar IPC Benchmark (Cortex-M7 dual ALU pipeline) */
void Bench_CPU_IPC(BenchResult *res)
{
    const uint32_t iterations = 20000;
    uint32_t r0 = 1, r1 = 2, r2 = 3, r3 = 4;
    uint32_t r4 = 5, r5 = 6, r6 = 7, r7 = 8;

    BENCH_START();
    for (uint32_t i = 0; i < iterations; i++) {
        /* 16 independent parallel ALU instructions allowing dual-issue execution */
        r0 += 1; r1 += 2; r2 += 3; r3 += 4;
        r4 += 5; r5 += 6; r6 += 7; r7 += 8;
        r0 ^= r1; r2 ^= r3; r4 ^= r5; r6 ^= r7;
        r1 += r0; r3 += r2; r5 += r4; r7 += r6;
    }
    g_sink = r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7;
    BENCH_STOP();

    /* 16 ALU ops + loop branch/counter = ~18 instructions per iteration */
    uint64_t total_instructions = (uint64_t)iterations * 18ULL;
    res->score = (res->cycles > 0) ? ((float)total_instructions / (float)res->cycles) : 1.0f; /* IPC */
}

/* 6. Hardware Bit Manipulation (RBIT, CLZ, REV instructions) */
static inline uint32_t bit_rbit(uint32_t val)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    uint32_t res;
    __asm__("rbit %0, %1" : "=r"(res) : "r"(val));
    return res;
#else
    val = (((val & 0xaaaaaaaa) >> 1) | ((val & 0x55555555) << 1));
    val = (((val & 0xcccccccc) >> 2) | ((val & 0x33333333) << 2));
    val = (((val & 0xf0f0f0f0) >> 4) | ((val & 0x0f0f0f0f) << 4));
    val = (((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8));
    return ((val >> 16) | (val << 16));
#endif
}

void Bench_CPU_BitOps(BenchResult *res)
{
    const uint32_t passes = 50000;
    uint32_t val = 0x12345678;
    uint32_t sum = 0;

    BENCH_START();
    for (uint32_t i = 0; i < passes; i++) {
        val = bit_rbit(val ^ i);
        sum += (uint32_t)__builtin_clz(val | 1);
        val = __builtin_bswap32(val + sum);
        sum += bit_rbit(val);
    }
    g_sink = sum + val;
    BENCH_STOP();

    /* 4 hardware bit manipulation operations per iteration */
    float total_ops = (float)passes * 4.0f;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_ops / time_sec / 1000000.0f) : 0.0f; /* MOps/s */
}

/* 7. Branch Predictor Stress Test (unpredictable vs predictable branching) */
void Bench_CPU_Branch(BenchResult *res)
{
    const uint32_t passes = 10000;
    uint32_t state = 0xACE1;
    uint32_t taken = 0;

    BENCH_START();
    for (uint32_t i = 0; i < passes; i++) {
        for (int j = 0; j < 32; j++) {
            /* Galois LFSR generating pseudo-random branch condition */
            state = (state >> 1) ^ (-(state & 1u) & 0xB400u);
            if (state & 1) {
                taken += j;
            } else {
                taken -= j;
            }
        }
    }
    g_sink = taken;
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)passes / time_sec / 1000.0f) : 0.0f; /* kPasses/s */
}

/* 8. Cryptographic SHA-256 Benchmark (64KB block hashing) */
static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void sha256_transform(uint32_t state[8], const uint8_t data[64])
{
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];
    for (int i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    for (int i = 16; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K256[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void Bench_CPU_SHA256(BenchResult *res)
{
    uint8_t block[64];
    for (int i = 0; i < 64; i++) block[i] = (uint8_t)(i * 17);

    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const int blocks = 1000; /* 64 KB total */

    BENCH_START();
    for (int b = 0; b < blocks; b++) {
        block[0] = (uint8_t)b;
        sha256_transform(state, block);
    }
    g_sink = state[0];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    float data_mb = (float)(blocks * 64) / (1024.0f * 1024.0f);
    res->score = (time_sec > 0) ? (data_mb / time_sec) : 0.0f; /* MB/s */
}
