/* Bare-metal USB 2.0 Full-Speed CDC ACM driver for STM32F072 */

#ifndef USB_F0_FS_H
#define USB_F0_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void USB_F0_Init(void);
void USB_F0_Poll(void);
bool USB_F0_IsConfigured(void);

void USB_F0_WriteChar(char c);
void USB_F0_WriteString(const char *str);
void USB_F0_WriteBytes(const uint8_t *data, size_t len);

bool USB_F0_HasChar(void);
char USB_F0_ReadChar(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_F0_FS_H */
