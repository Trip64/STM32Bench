/* Board Support Package for STM32F072B-DISCO (Discovery Kit) */

#ifndef BSP_DISCO_F072BD_H
#define BSP_DISCO_F072BD_H

#include <stdint.h>
#include <stdbool.h>

#define BSP_BOARD_NAME         "STM32F072B-DISCO"
#define BSP_MCU_NAME           "STM32F072RBT6"
#define BSP_CORE_FREQ_HZ       48000000UL

/* 4 User LEDs on Port C (Arranged as a Compass) */
#define BSP_LED_GREEN          0   /* PC9 - LD4 (North) */
#define BSP_LED_ORANGE         1   /* PC8 - LD3 (South) */
#define BSP_LED_RED            2   /* PC6 - LD5 (West) */
#define BSP_LED_BLUE           3   /* PC7 - LD6 (East) */
#define BSP_LED_COUNT          4

/* User Button B1 on PA0 (Active High) */
#define BSP_BUTTON_PIN         0   /* PA0 */

void BSP_Init(void);
void BSP_LED_Init(void);
void BSP_LED_On(uint8_t led);
void BSP_LED_Off(uint8_t led);
void BSP_LED_Toggle(uint8_t led);
bool BSP_Button_IsPressed(void);

#endif /* BSP_DISCO_F072BD_H */
