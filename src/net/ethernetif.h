/* STM32H7 Ethernet Network Interface Driver Header for lwIP */

#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t ethernetif_init(struct netif *netif);
void  ethernetif_input(struct netif *netif);
bool  ethernetif_is_link_up(void);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNETIF_H */
