/* JSON serialization implementation */

#include "json_output.h"
#include "pal.h"
#include <stdio.h>
#include <string.h>

size_t JSON_FormatBenchmarkResults(char *buf, size_t max_len)
{
    if (!buf || max_len < 64) return 0;

    int written = 0;
    int remaining = (int)max_len;

    BenchCategoryScores marks = Bench_GetScores();

    int n = snprintf(buf, remaining,
        "{\"suite\":\"%s\",\"chip\":\"%s\",\"core\":\"%s\",\"clock_mhz\":%lu,"
        "\"flash_kb\":%lu,\"ram_kb\":%lu,\"has_fpu\":%s,"
        "\"has_dp_fpu\":%s,\"has_dsp\":%s,\"has_cordic\":%s,"
        "\"has_fmac\":%s,\"has_dma2d\":%s,\"has_rng\":%s,\"icache\":%s,\"dcache\":%s,"
        "\"uptime_ms\":%lu,\"stm32mark\":%lu,"
        "\"scores\":{\"cpu\":%lu,\"fpu\":%lu,\"dsp\":%lu,\"gfx\":%lu,\"ai\":%lu,\"crypto\":%lu,\"io\":%lu,\"mem\":%lu},"
        "\"benchmarks\":[",
        Bench_GetSuiteName(),
        PAL_GetChipName(),
        PAL_GetCoreName(),
        (unsigned long)(PAL_GetCoreClockHz() / 1000000UL),
        (unsigned long)PAL_GetFlashSizeKB(),
        (unsigned long)PAL_GetRAMSizeKB(),
        PAL_HasFPU() ? "true" : "false",
        PAL_HasDPFPU() ? "true" : "false",
        PAL_HasDSP() ? "true" : "false",
        PAL_HasCORDIC() ? "true" : "false",
        PAL_HasFMAC() ? "true" : "false",
        PAL_HasDMA2D() ? "true" : "false",
        PAL_HasRNG() ? "true" : "false",
        PAL_HasICache() ? "true" : "false",
        PAL_HasDCache() ? "true" : "false",
        (unsigned long)PAL_GetMillis(),
        (unsigned long)marks.total,
        (unsigned long)marks.cpu,
        (unsigned long)marks.fpu,
        (unsigned long)marks.dsp,
        (unsigned long)marks.gfx,
        (unsigned long)marks.ai,
        (unsigned long)marks.crypto,
        (unsigned long)marks.io,
        (unsigned long)marks.mem
    );

    if (n < 0 || n >= remaining) return 0;
    written += n;
    remaining -= n;

    size_t count = Bench_GetCount();
    bool first = true;

    for (size_t i = 0; i < count; i++) {
        const BenchResult *r = Bench_GetResult(i);
        if (!r) continue;

        /* Reserve at least 10 bytes for closing `]}` */
        if (remaining < 64) break;

        n = snprintf(buf + written, remaining,
            "%s{\"id\":\"%s\",\"name\":\"%s\",\"category\":\"%s\","
            "\"cycles\":%lu,\"time_us\":%lu,"
            "\"score\":%.2f,\"unit\":\"%s\",\"available\":%s}",
            first ? "" : ",",
            r->id,
            r->name,
            r->category,
            (unsigned long)r->cycles,
            (unsigned long)r->time_us,
            (double)r->score,
            r->unit,
            r->available ? "true" : "false"
        );

        if (n < 0 || n >= remaining) break;
        written += n;
        remaining -= n;
        first = false;
    }

    n = snprintf(buf + written, remaining, "]}\n");
    if (n > 0 && n < remaining) {
        written += n;
    }

    return (size_t)written;
}

size_t JSON_FormatStatus(char *buf, size_t max_len)
{
    if (!buf || max_len < 32) return 0;
    int n = snprintf(buf, max_len,
        "{\"status\":\"ready\",\"clock_mhz\":%lu,\"uptime_ms\":%lu}\n",
        (unsigned long)(PAL_GetCoreClockHz() / 1000000UL),
        (unsigned long)PAL_GetMillis()
    );
    return (n > 0 && (size_t)n < max_len) ? (size_t)n : 0;
}
