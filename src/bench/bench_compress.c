/* Data Compression & Decompression Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include <string.h>

extern volatile uint32_t g_sink;

/* 1. Fast LZ4 Block Decompression */
#define LZ4_DEC_BUF_SIZE 2048
static uint8_t s_lz4_output[LZ4_DEC_BUF_SIZE];

/* Synthetic LZ4 compressed packet (literals and back-references) */
static const uint8_t s_lz4_sample[] = {
    0x1F, 0x41, 0x01, 0x00, 0x1F, 0x42, 0x01, 0x00, 0x1F, 0x43, 0x01, 0x00,
    0x40, 0x53, 0x54, 0x4D, 0x33, 0x32, 0x48, 0x37, 0x32, 0x33, 0x05, 0x00,
    0x2F, 0xAA, 0x55, 0x01, 0x00, 0x1F, 0xFF, 0x02, 0x00, 0x40, 0x61, 0x62,
    0x63, 0x64, 0x04, 0x00, 0x1F, 0x11, 0x22, 0x01, 0x00, 0x00
};

static int lz4_decompress_block(const uint8_t *src, int src_len, uint8_t *dst, int max_dst)
{
    int ip = 0, op = 0;
    while (ip < src_len && op < max_dst) {
        uint8_t token = src[ip++];
        int lit_len = token >> 4;

        if (lit_len == 15 && ip < src_len) lit_len += src[ip++];
        if (op + lit_len > max_dst || ip + lit_len > src_len) break;

        for (int i = 0; i < lit_len; i++) dst[op++] = src[ip++];
        if (ip >= src_len) break;

        uint16_t offset = (uint16_t)src[ip] | ((uint16_t)src[ip + 1] << 8);
        ip += 2;
        if (offset == 0) break;

        int match_len = (token & 0x0F) + 4;
        if (match_len == 19 && ip < src_len) match_len += src[ip++];
        if (op + match_len > max_dst) break;

        int ref = op - offset;
        if (ref < 0) break;

        for (int i = 0; i < match_len; i++) dst[op++] = dst[ref + i];
    }
    return op;
}

void Bench_Comp_LZ4(BenchResult *res)
{
    const int iterations = 1000;
    int total_decompressed = 0;

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        total_decompressed += lz4_decompress_block(s_lz4_sample, sizeof(s_lz4_sample),
                                                  s_lz4_output, sizeof(s_lz4_output));
    }
    g_sink = s_lz4_output[0] + total_decompressed;
    BENCH_STOP();

    float bytes = (float)total_decompressed;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}

/* 2. Run-Length Encoding (RLE) & Byte-Pack Compression */
#define RLE_BUF_SIZE 2048
static uint8_t s_rle_raw[RLE_BUF_SIZE];
static uint8_t s_rle_encoded[RLE_BUF_SIZE * 2];

void Bench_Comp_RLE(BenchResult *res)
{
    /* Generate typical telemetry with repeating sensor data */
    for (int i = 0; i < RLE_BUF_SIZE; i++) {
        s_rle_raw[i] = (uint8_t)((i / 16) * 17);
    }

    const int iterations = 500;
    int total_encoded = 0;

    BENCH_START();
    for (int iter = 0; iter < iterations; iter++) {
        int ip = 0, op = 0;
        while (ip < RLE_BUF_SIZE) {
            uint8_t val = s_rle_raw[ip];
            uint8_t count = 1;
            while (ip + count < RLE_BUF_SIZE && s_rle_raw[ip + count] == val && count < 255) {
                count++;
            }
            s_rle_encoded[op++] = count;
            s_rle_encoded[op++] = val;
            ip += count;
        }
        total_encoded += op;
    }
    g_sink = s_rle_encoded[0] + total_encoded;
    BENCH_STOP();

    float total_raw_bytes = (float)iterations * (float)RLE_BUF_SIZE;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_raw_bytes / (1024.0f * 1024.0f) / time_sec) : 0.0f; /* MB/s */
}
