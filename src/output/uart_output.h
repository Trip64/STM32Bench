/* UART console output formatter for STM32Benchmark */

#ifndef UART_OUTPUT_H
#define UART_OUTPUT_H

#include "bench_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

void UART_PrintBanner(void);
void UART_PrintSystemInfo(void);
void UART_PrintBenchmarkTable(void);
void UART_PrintResult(const BenchResult *r);

#ifdef __cplusplus
}
#endif

#endif /* UART_OUTPUT_H */
