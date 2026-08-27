/* Board Support Package implementation for NUCLEO-F072RB */

#include "bsp_nucleo_f072rb.h"
#include "stm32f072xb.h"

void BSP_LED_Init(void)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    __DSB();

    /* Configure PA5 as output (Green LED LD2) */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2)); /* General purpose output */
    GPIOA->OTYPER &= ~(1U << 5);      /* Push-pull */
    GPIOA->OSPEEDR |= (3U << (5 * 2));/* High speed */

    BSP_LED_Off(BSP_LED_GREEN);
}

void BSP_LED_On(uint8_t led)
{
    (void)led;
    GPIOA->BSRR = (1U << 5);
}

void BSP_LED_Off(uint8_t led)
{
    (void)led;
    GPIOA->BRR = (1U << 5);
}

void BSP_LED_Toggle(uint8_t led)
{
    (void)led;
    GPIOA->ODR ^= (1U << 5);
}

bool BSP_Button_IsPressed(void)
{
    /* PC13 is active LOW */
    return !(GPIOC->IDR & (1U << 13));
}

void BSP_Init(void)
{
    BSP_LED_Init();

    /* Enable GPIOC clock for Blue Button PC13 */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    __DSB();

    GPIOC->MODER &= ~(3U << (13 * 2)); /* Input mode */
    GPIOC->PUPDR &= ~(3U << (13 * 2)); /* No pull */
}
