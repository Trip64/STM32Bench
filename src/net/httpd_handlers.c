/* SSI, CGI, and Dynamic REST Endpoint handlers for lwIP HTTPD */

#include "httpd_handlers.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "pal.h"
#include "bench_engine.h"
#include "json_output.h"
#include <stdio.h>
#include <string.h>

static const char *s_ssi_tags[] = {
    "clk",         /* 0 */
    "chip",        /* 1 */
    "flash",       /* 2 */
    "ram",         /* 3 */
    "fpu",         /* 4 */
    "dsp",         /* 5 */
    "cordic",      /* 6 */
    "fmac",        /* 7 */
    "cache",       /* 8 */
    "bench_table", /* 9 */
    "stm32mark"    /* 10 */
};

#define SSI_TAG_COUNT (sizeof(s_ssi_tags) / sizeof(s_ssi_tags[0]))

static char s_json_response[16384];

static u16_t bench_ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    if (!pcInsert || iInsertLen <= 0) return 0;

    switch (iIndex) {
        case 0: /* clk */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%lu", (unsigned long)(PAL_GetCoreClockHz() / 1000000UL));

        case 1: /* chip */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", PAL_GetChipName());

        case 2: /* flash */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%lu", (unsigned long)PAL_GetFlashSizeKB());

        case 3: /* ram */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%lu", (unsigned long)PAL_GetRAMSizeKB());

        case 4: /* fpu */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", PAL_HasDPFPU() ? "DP-FPU Active" : "SP-FPU");

        case 5: /* dsp */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", PAL_HasDSP() ? "SIMD SMLAD" : "None");

        case 6: /* cordic */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", PAL_HasCORDIC() ? "CORDIC Engine" : "N/A");

        case 7: /* fmac */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", PAL_HasFMAC() ? "FMAC Filter" : "N/A");

        case 8: /* cache */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%s", (PAL_HasICache() && PAL_HasDCache()) ? "I+D Cache" : "Cached");

        case 9: {
            int written = 0;
            size_t count = Bench_GetCount();
            for (size_t i = 0; i < count; i++) {
                const BenchResult *r = Bench_GetResult(i);
                if (!r) continue;

                const char *hw = r->available ?
                    "<span class=\"tag-hw\">HW</span>" :
                    "<span class=\"tag-sw\">N/A</span>";

                int n = snprintf(pcInsert + written, iInsertLen - written,
                    "<tr data-cat=\"%s\">"
                    "<td><span class=\"cat c-%s\">%s</span></td>"
                    "<td><strong>%s</strong></td>"
                    "<td><span class=\"score\">%.2f</span></td>"
                    "<td class=\"mono\">%s</td>"
                    "<td class=\"mono\">%lu</td>"
                    "<td class=\"mono\">%lu</td>"
                    "<td>%s</td>"
                    "</tr>",
                    r->category, r->category, r->category,
                    r->name,
                    (double)r->score,
                    r->unit,
                    (unsigned long)r->cycles,
                    (unsigned long)r->time_us,
                    hw
                );
                if (n < 0 || (written + n) >= iInsertLen) break;
                written += n;
            }
            return (u16_t)written;
        }

        case 10: /* stm32mark */
            return (u16_t)snprintf(pcInsert, iInsertLen, "%lu", (unsigned long)Bench_GetTotalScore());

        default:
            return 0;
    }
}

static const char *cgi_run_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    (void)iIndex; (void)iNumParams; (void)pcParam; (void)pcValue;
    Bench_RunAll();
    return "/index.shtml";
}

static const tCGI s_cgi_handlers[] = {
    { "/run.cgi", cgi_run_handler }
};

int fs_open_custom(struct fs_file *file, const char *name)
{
    if (strcmp(name, "/api/benchmarks") == 0 || strcmp(name, "/data.json") == 0) {
        size_t json_len = JSON_FormatBenchmarkResults(s_json_response, sizeof(s_json_response));
        file->data = s_json_response;
        file->len  = (int)json_len;
        file->index = (int)json_len;
        file->flags = FS_FILE_FLAGS_CUSTOM;
        return 1;
    }
    return 0;
}

void fs_close_custom(struct fs_file *file)
{
    (void)file;
}

void HTTPD_RegisterHandlers(void)
{
    http_set_ssi_handler(bench_ssi_handler, s_ssi_tags, SSI_TAG_COUNT);

    http_set_cgi_handlers(s_cgi_handlers, sizeof(s_cgi_handlers) / sizeof(s_cgi_handlers[0]));
}
