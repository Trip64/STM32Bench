#include "usb_cdc.h"
#include "pal.h"
#include "bsp.h"
#include "bench_engine.h"
#include "uart_output.h"
#include "json_output.h"
#include <string.h>

#if ENABLE_USB_USER
#include "usb_f0_fs.h"
#endif

#ifndef ENABLE_UART
#define ENABLE_UART 1
#endif

#ifndef ENABLE_USB_USER
#define ENABLE_USB_USER 0
#endif

#if BENCH_SUITE_EXTENDED
static char s_json_buffer[16384];
#else
static char s_json_buffer[3072];
#endif

void USB_CDC_Init(void)
{
#if ENABLE_USB_USER
    USB_F0_Init();
#endif
}

void USB_CDC_SendString(const char *str)
{
    if (!str) return;
#if ENABLE_UART
    PAL_UART_WriteString(str);
#endif
#if ENABLE_USB_USER
    USB_F0_WriteString(str);
#endif
}

void USB_CDC_SendData(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return;
#if ENABLE_UART
    PAL_UART_WriteBytes(data, len);
#endif
#if ENABLE_USB_USER
    USB_F0_WriteBytes(data, len);
#endif
}

bool USB_CDC_IsConnected(void)
{
#if ENABLE_USB_USER
    return USB_F0_IsConfigured();
#else
    return true;
#endif
}

void USB_CDC_Poll(void)
{
#if ENABLE_USB_USER
    USB_F0_Poll();
#endif

    char c = 0;
    bool has_char = false;

#if ENABLE_USB_USER
    if (USB_F0_HasChar()) {
        c = USB_F0_ReadChar();
        has_char = true;
    }
#endif

#if ENABLE_UART
    if (!has_char && PAL_UART_HasChar()) {
        c = PAL_UART_ReadChar();
        has_char = true;
    }
#endif

    if (!has_char) return;

    if (c == '\r' || c == '\n') {
        USB_CDC_SendString("\r\nSTM32> ");
        return;
    } else {
        char echo[2] = {c, 0};
        USB_CDC_SendString(echo);
    }

    switch (c) {
    case 'r': case 'R': {
        USB_CDC_SendString("\r\n>> Running All Benchmarks...\r\n");
        Bench_RunAll();
        UART_PrintBenchmarkTable();
        USB_CDC_SendString("\r\n<!--JSON_START-->\r\n");
        size_t len = JSON_FormatBenchmarkResults(s_json_buffer, sizeof(s_json_buffer));
        USB_CDC_SendData((const uint8_t *)s_json_buffer, len);
        USB_CDC_SendString("\r\n<!--JSON_END-->\r\nSTM32> ");
        break;
    }
    case 'j': case 'J': {
        USB_CDC_SendString("\r\n<!--JSON_START-->\r\n");
        size_t len = JSON_FormatBenchmarkResults(s_json_buffer, sizeof(s_json_buffer));
        USB_CDC_SendData((const uint8_t *)s_json_buffer, len);
        USB_CDC_SendString("\r\n<!--JSON_END-->\r\nSTM32> ");
        break;
    }
    case 's': case 'S':
        USB_CDC_SendString("\r\n");
        UART_PrintSystemInfo();
        USB_CDC_SendString("STM32> ");
        break;
    case 'h': case 'H': case '?':
        USB_CDC_SendString("\r\n  r : Run benchmarks\r\n"
                           "  j : JSON output\r\n"
                           "  s : System specs\r\n"
                           "  h : This help\r\nSTM32> ");
        break;
    default:
        break;
    }
}
