/* Network stack bringup with static IP, lwIP HTTPD, and polling */

#include "net_init.h"
#include "ethernetif.h"
#include "httpd_handlers.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "lwip/apps/httpd.h"
#include "netif/ethernet.h"
#include "pal.h"

static struct netif s_netif;

u32_t sys_now(void)
{
    return PAL_GetMillis();
}

bool Net_Init(void)
{
    ip4_addr_t ipaddr, netmask, gw;

    /* Static IP: 192.168.1.100 / 24 */
    IP4_ADDR(&ipaddr, 192, 168, 1, 100);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 1, 1);

    /* Initialize lwIP core */
    lwip_init();

    /* Add network interface */
    if (netif_add(&s_netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input) == NULL) {
        return false;
    }

    netif_set_default(&s_netif);
    netif_set_up(&s_netif);

    /* Start embedded HTTP server */
    httpd_init();

    /* Register SSI & CGI handlers */
    HTTPD_RegisterHandlers();

    return true;
}

void Net_Poll(void)
{
    /* Receive any incoming packets from Ethernet DMA */
    ethernetif_input(&s_netif);

    /* Process lwIP timers (TCP timeouts, retransmissions) */
    sys_check_timeouts();
}

bool Net_IsConnected(void)
{
    return ethernetif_is_link_up();
}
