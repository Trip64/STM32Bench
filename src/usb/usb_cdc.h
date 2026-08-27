/* USB CDC / Serial Communication Transport Interface */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void USB_CDC_Init(void);
void USB_CDC_Poll(void);
void USB_CDC_SendString(const char *str);
void USB_CDC_SendData(const uint8_t *data, size_t len);
bool USB_CDC_IsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_H */
