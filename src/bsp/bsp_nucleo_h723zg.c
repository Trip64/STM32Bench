/* BSP implementation for NUCLEO-H723ZG board LEDs, button, pins */

#include "bsp_nucleo_h723zg.h"
#include "stm32h7xx.h"

void BSP_LED_Init(void)
{
    /* Enable GPIOB and GPIOE clock */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOEEN;
    __DSB();

    /* Configure PB0 (LD1 Green) as Output */
    GPIOB->MODER &= ~(3U << (0 * 2));
    GPIOB->MODER |=  (1U << (0 * 2));
    GPIOB->OTYPER &= ~(1U << 0);
    GPIOB->OSPEEDR |= (3U << (0 * 2));
    GPIOB->PUPDR &= ~(3U << (0 * 2));

    /* Configure PE1 (LD2 Yellow) as Output */
    GPIOE->MODER &= ~(3U << (1 * 2));
    GPIOE->MODER |=  (1U << (1 * 2));
    GPIOE->OTYPER &= ~(1U << 1);
    GPIOE->OSPEEDR |= (3U << (1 * 2));
    GPIOE->PUPDR &= ~(3U << (1 * 2));

    /* Configure PB14 (LD3 Red) as Output */
    GPIOB->MODER &= ~(3U << (14 * 2));
    GPIOB->MODER |=  (1U << (14 * 2));
    GPIOB->OTYPER &= ~(1U << 14);
    GPIOB->OSPEEDR |= (3U << (14 * 2));
    GPIOB->PUPDR &= ~(3U << (14 * 2));

    /* Turn off all LEDs initially */
    BSP_LED_Off(BSP_LED_GREEN);
    BSP_LED_Off(BSP_LED_YELLOW);
    BSP_LED_Off(BSP_LED_RED);
}

void BSP_LED_On(uint8_t led)
{
    switch(led) {
        case BSP_LED_GREEN:  GPIOB->BSRR = (1U << 0); break;
        case BSP_LED_YELLOW: GPIOE->BSRR = (1U << 1); break;
        case BSP_LED_RED:    GPIOB->BSRR = (1U << 14); break;
        default: break;
    }
}

void BSP_LED_Off(uint8_t led)
{
    switch(led) {
        case BSP_LED_GREEN:  GPIOB->BSRR = (1U << (0 + 16)); break;
        case BSP_LED_YELLOW: GPIOE->BSRR = (1U << (1 + 16)); break;
        case BSP_LED_RED:    GPIOB->BSRR = (1U << (14 + 16)); break;
        default: break;
    }
}

void BSP_LED_Toggle(uint8_t led)
{
    switch(led) {
        case BSP_LED_GREEN:  GPIOB->ODR ^= (1U << 0); break;
        case BSP_LED_YELLOW: GPIOE->ODR ^= (1U << 1); break;
        case BSP_LED_RED:    GPIOB->ODR ^= (1U << 14); break;
        default: break;
    }
}

static void BSP_Button_Init(void)
{
    /* Enable GPIOC clock */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    __DSB();

    /* Configure PC13 as Input */
    GPIOC->MODER &= ~(3U << (13 * 2));
    GPIOC->PUPDR &= ~(3U << (13 * 2)); /* No pull */
}

bool BSP_Button_IsPressed(void)
{
    /* PC13 is active HIGH on Nucleo-144 */
    return (GPIOC->IDR & (1U << 13)) != 0;
}

void BSP_Init(void)
{
    BSP_LED_Init();
    BSP_Button_Init();
}
