/* Board Support Package for NUCLEO-H723ZG */

#ifndef BSP_NUCLEO_H723ZG_H
#define BSP_NUCLEO_H723ZG_H

#include <stdint.h>
#include <stdbool.h>

#define BSP_BOARD_NAME         "NUCLEO-H723ZG"
#define BSP_MCU_NAME           "STM32H723ZGT6"
#define BSP_CORE_FREQ_HZ       550000000UL

/* LED indices */
#define BSP_LED_GREEN          0   /* PB0  - LD1 */
#define BSP_LED_YELLOW         1   /* PE1  - LD2 */
#define BSP_LED_RED            2   /* PB14 - LD3 */
#define BSP_LED_COUNT          3

/* Button */
#define BSP_BUTTON_PIN         13  /* PC13 - Blue Push Button */

/* Ethernet PHY LAN8742 */
#define LAN8742_PHY_ADDRESS    0x00U

void BSP_Init(void);
void BSP_LED_Init(void);
void BSP_LED_On(uint8_t led);
void BSP_LED_Off(uint8_t led);
void BSP_LED_Toggle(uint8_t led);
bool BSP_Button_IsPressed(void);

#endif /* BSP_NUCLEO_H723ZG_H */
