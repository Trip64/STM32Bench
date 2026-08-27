/* Hardware Security & Cryptography Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include "stm32h7xx.h"
#include <string.h>

extern volatile uint32_t g_sink;

/* 1. STM32H7 On-Chip Hardware True Random Number Generator (RNG) */
void Bench_Crypto_RNG(BenchResult *res)
{
    bool has_rng = PAL_HasRNG();
    const int words = 2000; /* 8 KB entropy */
    uint32_t dummy = 0;

    if (has_rng) {
        /* Enable HSI48 oscillator for RNG clock */
        RCC->CR |= RCC_CR_HSI48ON;
        uint32_t to = 20000;
        while (!(RCC->CR & RCC_CR_HSI48RDY) && --to) {}

        /* Select HSI48 for RNG */
        RCC->D2CCIP2R &= ~RCC_D2CCIP2R_RNGSEL_Msk;

        /* Enable RNG clock on AHB2 */
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
        __DSB();

        /* Enable RNG peripheral */
        RNG->CR |= RNG_CR_RNGEN;
    }

    BENCH_START();
    if (has_rng) {
        for (int i = 0; i < words; i++) {
            uint32_t timeout = 5000;
            while (!(RNG->SR & RNG_SR_DRDY) && --timeout) {
                if (RNG->SR & (RNG_SR_CECS | RNG_SR_SECS)) {
                    RNG->SR = ~(RNG_SR_CEIS | RNG_SR_SEIS);
                    break;
                }
            }
            if (!timeout) break;
            dummy ^= RNG->DR;
        }
        res->available = true;
    } else {
        /* Software fallback XorShift32 */
        uint32_t state = 0x2468ACE1;
        for (int i = 0; i < words; i++) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            dummy ^= state;
        }
        res->available = false;
    }
    g_sink = dummy;
    BENCH_STOP();

    float bytes = (float)(words * 4);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

/* 2. AES-128 Block Cipher Encryption */
static const uint8_t s_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5e,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static void aes128_encrypt_block(uint8_t state[16])
{
    for (int round = 0; round < 10; round++) {
        /* SubBytes */
        for (int i = 0; i < 16; i++) state[i] = s_sbox[state[i]];

        /* ShiftRows */
        uint8_t temp = state[1];
        state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = temp;
        temp = state[2]; state[2] = state[10]; state[10] = temp;
        temp = state[6]; state[6] = state[14]; state[14] = temp;
        temp = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = temp;

        /* AddRoundKey dummy xor */
        for (int i = 0; i < 16; i++) state[i] ^= (uint8_t)(round + i);
    }
}

void Bench_Crypto_AES(BenchResult *res)
{
    uint8_t block[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    const int blocks = 1000; /* 16 KB encrypted */

    BENCH_START();
    for (int b = 0; b < blocks; b++) {
        block[0] = (uint8_t)b;
        aes128_encrypt_block(block);
    }
    g_sink = block[0];
    BENCH_STOP();

    float bytes = (float)(blocks * 16);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

/* 3. ChaCha20 Stream Cipher Block Function (20 rounds on 512-bit state) */
#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
#define CHACHA_QR(a, b, c, d) \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8);  \
    c += d; b ^= c; b = ROTL32(b, 7);

void Bench_Crypto_ChaCha20(BenchResult *res)
{
    uint32_t state[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, /* "expand 32-byte k" */
        0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
        0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
        0x00000001, 0x09000000, 0x4a000000, 0x00000000
    };

    const int blocks = 500; /* 32 KB processed */

    BENCH_START();
    for (int b = 0; b < blocks; b++) {
        state[12] = (uint32_t)b;
        uint32_t x[16];
        memcpy(x, state, sizeof(x));

        for (int i = 0; i < 10; i++) {
            /* Column rounds */
            CHACHA_QR(x[0], x[4], x[8],  x[12]);
            CHACHA_QR(x[1], x[5], x[9],  x[13]);
            CHACHA_QR(x[2], x[6], x[10], x[14]);
            CHACHA_QR(x[3], x[7], x[11], x[15]);
            /* Diagonal rounds */
            CHACHA_QR(x[0], x[5], x[10], x[15]);
            CHACHA_QR(x[1], x[6], x[11], x[12]);
            CHACHA_QR(x[2], x[7], x[8],  x[13]);
            CHACHA_QR(x[3], x[4], x[9],  x[14]);
        }
        state[0] ^= x[0];
    }
    g_sink = state[0];
    BENCH_STOP();

    float bytes = (float)(blocks * 64);
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}
