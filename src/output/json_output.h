/* JSON serialization for benchmark results and system telemetry */

#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include <stddef.h>
#include "bench_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generate full system and benchmark JSON into buffer */
size_t JSON_FormatBenchmarkResults(char *buf, size_t max_len);

/* Generate lightweight status JSON */
size_t JSON_FormatStatus(char *buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* JSON_OUTPUT_H */
