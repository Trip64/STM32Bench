/* Platform Abstraction Layer (PAL) Interface */

#ifndef PAL_H
#define PAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System Initialization */
void     PAL_Init(void);

/* Clock & Timing */
uint32_t PAL_GetCoreClockHz(void);
uint32_t PAL_GetCycleCount(void);
uint32_t PAL_GetMillis(void);
void     PAL_DelayMs(uint32_t ms);

/* Hardware Feature Detection */
bool     PAL_HasFPU(void);
bool     PAL_HasDPFPU(void);
bool     PAL_HasDSP(void);
bool     PAL_HasCORDIC(void);
bool     PAL_HasFMAC(void);
bool     PAL_HasICache(void);
bool     PAL_HasDCache(void);
bool     PAL_HasDMA2D(void);
bool     PAL_HasRNG(void);

/* Cache Management */
void     PAL_EnableICache(void);
void     PAL_DisableICache(void);
void     PAL_EnableDCache(void);
void     PAL_DisableDCache(void);
void     PAL_CleanDCache(void);
void     PAL_CleanInvalidateDCache(void);

/* Chip & Architecture Info */
const char* PAL_GetChipName(void);
const char* PAL_GetCoreName(void);
uint32_t    PAL_GetFlashSizeKB(void);
uint32_t    PAL_GetRAMSizeKB(void);
uint32_t    PAL_GetCPUID(void);
uint16_t    PAL_GetDeviceID(void);
uint16_t    PAL_GetRevisionID(void);
void        PAL_GetUID(uint32_t uid[3]);

/* Low-level UART for Debug/Transport */
void     PAL_UART_Init(uint32_t baudrate);
void     PAL_UART_WriteChar(char c);
void     PAL_UART_WriteString(const char* str);
void     PAL_UART_WriteBytes(const uint8_t* data, size_t len);
bool     PAL_UART_HasChar(void);
char     PAL_UART_ReadChar(void);

/* Board LED and Button indicators */
void     PAL_LED_Init(void);
void     PAL_LED_Set(int led_index, bool state);
void     PAL_LED_Toggle(int led_index);
bool     PAL_Button_Read(void);

#ifdef __cplusplus
}
#endif

#endif /* PAL_H */
