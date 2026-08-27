/* Board Support Package for STM32F072B-DISCO */

#include "bsp_disco_f072bd.h"
#include "stm32f072xb.h"

/* LED Pin Mapping on Port C:
   LD3 (Orange) = PC8
   LD4 (Green)  = PC9
   LD5 (Red)    = PC6
   LD6 (Blue)   = PC7
*/
static const uint8_t s_led_pins[BSP_LED_COUNT] = {
    9, /* BSP_LED_GREEN  (LD4) */
    8, /* BSP_LED_ORANGE (LD3) */
    6, /* BSP_LED_RED    (LD5) */
    7  /* BSP_LED_BLUE   (LD6) */
};

void BSP_LED_Init(void)
{
    /* Enable GPIOC clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    __DSB();

    /* Configure PC6, PC7, PC8, PC9 as General Purpose Output (MODER = 01) */
    for (int i = 0; i < BSP_LED_COUNT; i++) {
        uint8_t pin = s_led_pins[i];
        GPIOC->MODER &= ~(3U << (pin * 2));
        GPIOC->MODER |=  (1U << (pin * 2));   /* Output mode */
        GPIOC->OTYPER &= ~(1U << pin);         /* Push-pull */
        GPIOC->OSPEEDR &= ~(3U << (pin * 2));  /* Low speed */
        GPIOC->PUPDR &= ~(3U << (pin * 2));    /* No pull-up, pull-down */
        GPIOC->BRR = (1U << pin);              /* Turn OFF initially */
    }
}

void BSP_LED_On(uint8_t led)
{
    if (led < BSP_LED_COUNT) {
        GPIOC->BSRR = (1U << s_led_pins[led]);
    }
}

void BSP_LED_Off(uint8_t led)
{
    if (led < BSP_LED_COUNT) {
        GPIOC->BRR = (1U << s_led_pins[led]);
    }
}

void BSP_LED_Toggle(uint8_t led)
{
    if (led < BSP_LED_COUNT) {
        GPIOC->ODR ^= (1U << s_led_pins[led]);
    }
}

static void BSP_Button_Init(void)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    __DSB();

    /* Configure PA0 as Input (MODER = 00) with Pull-Down (PUPDR = 10) */
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |=  (2U << (0 * 2)); /* Pull-down */
}

bool BSP_Button_IsPressed(void)
{
    /* B1 User Button on STM32F072B-DISCO is Active HIGH (connected to VDD) */
    return (GPIOA->IDR & (1U << 0)) != 0;
}

void BSP_Init(void)
{
    BSP_LED_Init();
    BSP_Button_Init();

    /* Power-on sweep animation across the 4 compass LEDs */
    for (int cycle = 0; cycle < 2; cycle++) {
        for (int i = 0; i < BSP_LED_COUNT; i++) {
            BSP_LED_On(i);
            for (volatile int d = 0; d < 80000; d++) {}
            BSP_LED_Off(i);
        }
    }
}
