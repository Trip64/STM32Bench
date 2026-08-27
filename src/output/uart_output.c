#include "uart_output.h"
#include "pal.h"
#include "usb_cdc.h"
#include <stdio.h>

#define PAL_UART_WriteString USB_CDC_SendString

void UART_PrintBanner(void)
{
    PAL_UART_WriteString("\r\n");
    PAL_UART_WriteString("===============================================================\r\n");
    PAL_UART_WriteString("       STM32 HIGH-PERFORMANCE BENCHMARK SUITE (Bare-Metal)     \r\n");
    PAL_UART_WriteString("===============================================================\r\n");
}

void UART_PrintSystemInfo(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "Target MCU : %s\r\n", PAL_GetChipName());
    PAL_UART_WriteString(buf);
    snprintf(buf, sizeof(buf), "Core CPU   : %s @ %lu MHz\r\n",
             PAL_GetCoreName(), (unsigned long)(PAL_GetCoreClockHz() / 1000000UL));
    PAL_UART_WriteString(buf);
    snprintf(buf, sizeof(buf), "Memory     : Flash %lu KB | RAM %lu KB\r\n",
             (unsigned long)PAL_GetFlashSizeKB(), (unsigned long)PAL_GetRAMSizeKB());
    PAL_UART_WriteString(buf);
    snprintf(buf, sizeof(buf), "FPU        : %s | DP-FPU: %s | DSP SIMD: %s\r\n",
             PAL_HasFPU() ? "YES" : "NO",
             PAL_HasDPFPU() ? "YES" : "NO",
             PAL_HasDSP() ? "YES" : "NO");
    PAL_UART_WriteString(buf);
    snprintf(buf, sizeof(buf), "Hardware   : CORDIC: %s | FMAC: %s\r\n",
             PAL_HasCORDIC() ? "YES" : "NO",
             PAL_HasFMAC() ? "YES" : "NO");
    PAL_UART_WriteString(buf);
    snprintf(buf, sizeof(buf), "Caches     : I-Cache: %s | D-Cache: %s\r\n",
             PAL_HasICache() ? "ON" : "OFF",
             PAL_HasDCache() ? "ON" : "OFF");
    PAL_UART_WriteString(buf);
    PAL_UART_WriteString("---------------------------------------------------------------\r\n");
}

void UART_PrintResult(const BenchResult *r)
{
    if (!r) return;
    char line[128];
    if (!r->available) {
        snprintf(line, sizeof(line), "[%-6s] %-28s : NOT SUPPORTED\r\n", r->category, r->name);
    } else {
        int score_int = (int)r->score;
        int score_frac = (int)((r->score - (float)score_int) * 100.0f);
        if (score_frac < 0) score_frac = -score_frac;

        snprintf(line, sizeof(line), "[%-6s] %-26s : %7d.%02d %-10s (%lu us)\r\n",
                 r->category, r->name, score_int, score_frac, r->unit, (unsigned long)r->time_us);
    }
    PAL_UART_WriteString(line);
}

void UART_PrintBenchmarkTable(void)
{
    PAL_UART_WriteString("\r\n--- BENCHMARK RESULTS ---\r\n");
    size_t count = Bench_GetCount();
    const char *last_cat = "";
    for (size_t i = 0; i < count; i++) {
        const BenchResult *r = Bench_GetResult(i);
        if (r && r->category != last_cat) {
            PAL_UART_WriteString("---------------------------------------------------------------\r\n");
            last_cat = r->category;
        }
        UART_PrintResult(r);
    }
    PAL_UART_WriteString("===============================================================\r\n");

    BenchCategoryScores marks = Bench_GetScores();
    char score_buf[128];
    PAL_UART_WriteString("                     COMPOSITE PERFORMANCE INDEX\r\n");
    PAL_UART_WriteString("---------------------------------------------------------------\r\n");
    snprintf(score_buf, sizeof(score_buf), "  CPU Mark          : %6lu pts\r\n", (unsigned long)marks.cpu);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  FPU Mark          : %6lu pts\r\n", (unsigned long)marks.fpu);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  DSP & Coprocessor : %6lu pts\r\n", (unsigned long)marks.dsp);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  2D/3D Graphics    : %6lu pts\r\n", (unsigned long)marks.gfx);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  TinyML & Edge AI  : %6lu pts\r\n", (unsigned long)marks.ai);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  Hardware Crypto   : %6lu pts\r\n", (unsigned long)marks.crypto);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  GPIO, NVIC & DMA  : %6lu pts\r\n", (unsigned long)marks.io);
    PAL_UART_WriteString(score_buf);
    snprintf(score_buf, sizeof(score_buf), "  Memory Subsystem  : %6lu pts\r\n", (unsigned long)marks.mem);
    PAL_UART_WriteString(score_buf);
    PAL_UART_WriteString("---------------------------------------------------------------\r\n");
    snprintf(score_buf, sizeof(score_buf), "  TOTAL STM32MARK   : %6lu pts\r\n", (unsigned long)marks.total);
    PAL_UART_WriteString(score_buf);
    PAL_UART_WriteString("===============================================================\r\n\r\n");
}
