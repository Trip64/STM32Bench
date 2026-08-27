/* STM32 Benchmark Suite - Main Application */

#include "pal.h"
#include "bench_engine.h"
#include "uart_output.h"
#include "json_output.h"
#include "usb_cdc.h"
#include "bsp.h"

#if ENABLE_ETHERNET
#include "net_init.h"
#endif

int main(void)
{
    PAL_Init();
    USB_CDC_Init();

    UART_PrintBanner();
    UART_PrintSystemInfo();

    PAL_UART_WriteString("[INIT] Initializing benchmark test suite...\r\n");
    Bench_Init();

#if ENABLE_ETHERNET
    PAL_UART_WriteString("[NET] Starting Ethernet & lwIP HTTP web server...\r\n");
    bool net_ok = Net_Init();
    if (net_ok) {
        PAL_UART_WriteString("[NET] Web Dashboard active at: http://192.168.1.100\r\n");
        if (Net_IsConnected())
            PAL_UART_WriteString("[NET] Ethernet Link: UP (LAN8742 100Mbps)\r\n");
        else
            PAL_UART_WriteString("[NET] Ethernet Link: STANDBY (Waiting for cable)\r\n");
    } else {
        PAL_UART_WriteString("[NET] Ethernet bypassed. Running in USB serial mode.\r\n");
    }
#else
    PAL_UART_WriteString("[NET] Ethernet disabled at compile time (-DENABLE_ETHERNET=0).\r\n");
#endif

    PAL_UART_WriteString("\r\n======================================================\r\n");
    PAL_UART_WriteString("  STM32 Benchmark Suite is READY!\r\n");
    PAL_UART_WriteString("  Serial Commands: 'r' = Run, 'j' = JSON, 's' = Specs\r\n");
#if ENABLE_ETHERNET
    PAL_UART_WriteString("  Connect via USB Web Dashboard or http://192.168.1.100\r\n");
#else
    PAL_UART_WriteString("  Connect via USB Web Dashboard (tools/usb_dashboard.html)\r\n");
#endif
    PAL_UART_WriteString("======================================================\r\n\r\n");

    uint32_t last_heartbeat = PAL_GetMillis();
    bool baseline_run_done = false;

    while (1) {
        USB_CDC_Poll();
#if ENABLE_ETHERNET
        Net_Poll();
#endif

        if (BSP_Button_IsPressed()) {
            while (BSP_Button_IsPressed()) PAL_DelayMs(10);
            PAL_UART_WriteString("\r\n>> [BUTTON] Running benchmarks...\r\n");
            Bench_RunAll();
            UART_PrintBenchmarkTable();
        }

        if (!baseline_run_done) {
            baseline_run_done = true;
            PAL_UART_WriteString(">> Initial benchmark pass...\r\n");
            Bench_RunAll();
            UART_PrintBenchmarkTable();
            PAL_UART_WriteString("\r\n>> 'r' = re-run, 'j' = JSON, 's' = specs\r\n\r\n");
        }

        /* Heartbeat: toggle green LED every 500ms */
        if (PAL_GetMillis() - last_heartbeat >= 500) {
            last_heartbeat = PAL_GetMillis();
            BSP_LED_Toggle(BSP_LED_GREEN);
        }
    }

    return 0;
}
