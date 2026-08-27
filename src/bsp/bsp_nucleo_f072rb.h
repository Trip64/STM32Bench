/* Board Support Package for NUCLEO-F072RB */

#ifndef BSP_NUCLEO_F072RB_H
#define BSP_NUCLEO_F072RB_H

#include <stdint.h>
#include <stdbool.h>

#define BSP_BOARD_NAME         "NUCLEO-F072RB"
#define BSP_MCU_NAME           "STM32F072RBT6"
#define BSP_CORE_FREQ_HZ       48000000UL

/* Single User LED LD2 on PA5 */
#define BSP_LED_GREEN          0
#define BSP_LED_YELLOW         0
#define BSP_LED_RED            0
#define BSP_LED_COUNT          1

/* Button */
#define BSP_BUTTON_PIN         13  /* PC13 - Blue Push Button */

void BSP_Init(void);
void BSP_LED_Init(void);
void BSP_LED_On(uint8_t led);
void BSP_LED_Off(uint8_t led);
void BSP_LED_Toggle(uint8_t led);
bool BSP_Button_IsPressed(void);

#endif /* BSP_NUCLEO_F072RB_H */
