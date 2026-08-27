/* Unified Board Support Package interface */

#ifndef BSP_H
#define BSP_H

#if defined(BOARD_STM32F072_DISCO) || defined(BOARD_STM32F072B_DISCO) || defined(STM32F072xB)
  #include "bsp_disco_f072bd.h"
#elif defined(BOARD_NUCLEO_F072RB)
  #include "bsp_nucleo_f072rb.h"
#else
  #include "bsp_nucleo_h723zg.h"
#endif

#endif /* BSP_H */
