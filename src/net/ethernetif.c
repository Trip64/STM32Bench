/* STM32H7 Ethernet MAC and LAN8742A PHY Driver for lwIP */

#include "ethernetif.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "stm32h7xx.h"
#include "pal.h"
#include <string.h>

#define ETH_RX_BUFFER_SIZE  1536
#define ETH_TX_BUFFER_SIZE  1536
#define ETH_RX_DESC_CNT     4
#define ETH_TX_DESC_CNT     4

/* Enhanced DMA Descriptor Structure for STM32H7 */
typedef struct {
    volatile uint32_t DESC0;
    volatile uint32_t DESC1;
    volatile uint32_t DESC2;
    volatile uint32_t DESC3;
} ETH_DMADescTypeDef;

/* Place descriptors & buffers in D2 SRAM1 (0x30000000) non-cacheable region */
static ETH_DMADescTypeDef s_dma_rx_desc[ETH_RX_DESC_CNT] __attribute__((section(".eth_desc"), aligned(32)));
static ETH_DMADescTypeDef s_dma_tx_desc[ETH_TX_DESC_CNT] __attribute__((section(".eth_desc"), aligned(32)));
static uint8_t s_rx_buffers[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE] __attribute__((section(".eth_buffers"), aligned(32)));
static uint8_t s_tx_buffers[ETH_TX_DESC_CNT][ETH_TX_BUFFER_SIZE] __attribute__((section(".eth_buffers"), aligned(32)));

static uint32_t s_rx_desc_idx = 0;
static uint32_t s_tx_desc_idx = 0;
static bool s_link_status = false;

#define LAN8742_PHY_ADDR    0x00U
#define PHY_BCR             0x00U /* Basic Control Register */
#define PHY_BSR             0x01U /* Basic Status Register */
#define PHY_BSR_LINK_STATUS (1U << 2)

/* SMI Read PHY Register with timeout */
static uint16_t ETH_PHY_Read(uint8_t phy_addr, uint8_t reg_addr)
{
    uint32_t to = 50000;
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) && --to) {}
    if (!to) return 0;

    /* CR = 4 (divider for 200-250 MHz CSR clock) */
    ETH->MACMDIOAR = ((uint32_t)phy_addr << 21) |
                     ((uint32_t)reg_addr << 16) |
                     ETH_MACMDIOAR_CR_DIV102 |
                     ETH_MACMDIOAR_MOC_RD | /* Read command */
                     ETH_MACMDIOAR_MB;

    to = 50000;
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) && --to) {}
    if (!to) return 0;

    return (uint16_t)(ETH->MACMDIODR & 0xFFFF);
}

/* SMI Write PHY Register with timeout */
static void ETH_PHY_Write(uint8_t phy_addr, uint8_t reg_addr, uint16_t val)
{
    uint32_t to = 50000;
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) && --to) {}
    if (!to) return;

    ETH->MACMDIODR = val;
    ETH->MACMDIOAR = ((uint32_t)phy_addr << 21) |
                     ((uint32_t)reg_addr << 16) |
                     ETH_MACMDIOAR_CR_DIV102 |
                     ETH_MACMDIOAR_MOC_WR | /* Write command */
                     ETH_MACMDIOAR_MB;

    to = 50000;
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) && --to) {}
}

/* Low-level GPIO and RMII hardware setup */
static void ETH_GPIO_Init(void)
{
    /* Enable GPIOA, GPIOC, GPIOG clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIOGEN;
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    __DSB();

    /* Select RMII interface in SYSCFG PMCR */
    SYSCFG->PMCR = (SYSCFG->PMCR & ~SYSCFG_PMCR_EPIS_SEL) | SYSCFG_PMCR_EPIS_SEL_2;

    /* Configure Pins for RMII (AF11):
     * PA1 (REF_CLK), PA2 (MDIO), PA7 (CRS_DV)
     * PC1 (MDC), PC4 (RXD0), PC5 (RXD1)
     * PG11 (TX_EN), PG13 (TXD0), PG14 (TXD1)
     */
    /* PA1, PA2, PA7 */
    GPIOA->MODER &= ~((3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (7 * 2)));
    GPIOA->MODER |=  ((2U << (1 * 2)) | (2U << (2 * 2)) | (2U << (7 * 2)));
    GPIOA->AFR[0] |= (11U << (1 * 4)) | (11U << (2 * 4)) | (11U << (7 * 4));
    GPIOA->OSPEEDR |= (3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (7 * 2));

    /* PC1, PC4, PC5 */
    GPIOC->MODER &= ~((3U << (1 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)));
    GPIOC->MODER |=  ((2U << (1 * 2)) | (2U << (4 * 2)) | (2U << (5 * 2)));
    GPIOC->AFR[0] |= (11U << (1 * 4)) | (11U << (4 * 4)) | (11U << (5 * 4));
    GPIOC->OSPEEDR |= (3U << (1 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2));

    /* PG11, PG13, PG14 */
    GPIOG->MODER &= ~((3U << (11 * 2)) | (3U << (13 * 2)) | (3U << (14 * 2)));
    GPIOG->MODER |=  ((2U << (11 * 2)) | (2U << (13 * 2)) | (2U << (14 * 2)));
    GPIOG->AFR[1] |= (11U << ((11 - 8) * 4)) | (11U << ((13 - 8) * 4)) | (11U << ((14 - 8) * 4));
    GPIOG->OSPEEDR |= (3U << (11 * 2)) | (3U << (13 * 2)) | (3U << (14 * 2));

    /* Enable Ethernet MAC clocks in AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_ETH1MACEN | RCC_AHB1ENR_ETH1TXEN | RCC_AHB1ENR_ETH1RXEN;
    __DSB();
}

bool ethernetif_is_link_up(void)
{
    uint16_t bsr = ETH_PHY_Read(LAN8742_PHY_ADDR, PHY_BSR);
    s_link_status = (bsr & PHY_BSR_LINK_STATUS) != 0;
    return s_link_status;
}

static err_t low_level_init(struct netif *netif)
{
    ETH_GPIO_Init();

    /* Reset Ethernet DMA */
    ETH->DMAMR |= ETH_DMAMR_SWR;
    uint32_t timeout = 100000;
    while ((ETH->DMAMR & ETH_DMAMR_SWR) && --timeout) {}

    /* Reset PHY */
    ETH_PHY_Write(LAN8742_PHY_ADDR, PHY_BCR, 0x8000);
    PAL_DelayMs(50);
    ETH_PHY_Write(LAN8742_PHY_ADDR, PHY_BCR, 0x1200); /* Auto-negotiate enable */

    /* Setup RX DMA Ring */
    for (int i = 0; i < ETH_RX_DESC_CNT; i++) {
        s_dma_rx_desc[i].DESC0 = (uint32_t)&s_rx_buffers[i][0];
        s_dma_rx_desc[i].DESC1 = 0;
        s_dma_rx_desc[i].DESC2 = 0;
        s_dma_rx_desc[i].DESC3 = 0x80000000U | (1U << 24); /* OWN bit + IOC */
    }
    ETH->DMACRDLAR = (uint32_t)&s_dma_rx_desc[0];
    ETH->DMACRDRLR = ETH_RX_DESC_CNT - 1;
    ETH->DMACRDTPR = (uint32_t)&s_dma_rx_desc[ETH_RX_DESC_CNT - 1];

    /* Setup TX DMA Ring */
    for (int i = 0; i < ETH_TX_DESC_CNT; i++) {
        s_dma_tx_desc[i].DESC0 = (uint32_t)&s_tx_buffers[i][0];
        s_dma_tx_desc[i].DESC1 = 0;
        s_dma_tx_desc[i].DESC2 = 0;
        s_dma_tx_desc[i].DESC3 = 0; /* Not owned by DMA yet */
    }
    ETH->DMACTDLAR = (uint32_t)&s_dma_tx_desc[0];
    ETH->DMACTDRLR = ETH_TX_DESC_CNT - 1;
    ETH->DMACTDTPR = (uint32_t)&s_dma_tx_desc[0];

    /* Configure MAC CR (Full Duplex, 100M, Fast Ethernet) */
    ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_TE | ETH_MACCR_RE;

    /* Start DMA Transmission and Reception */
    ETH->DMACTCR |= ETH_DMACTCR_ST;
    ETH->DMACRCR |= ETH_DMACRCR_SR | (ETH_RX_BUFFER_SIZE << 1);

    /* Set MAC Address in netif */
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x80;
    netif->hwaddr[2] = 0xE1;
    netif->hwaddr[3] = 0x72;
    netif->hwaddr[4] = 0x30;
    netif->hwaddr[5] = 0x01;
    netif->hwaddr_len = 6;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* Set MAC address in MAC register */
    ETH->MACA0LR = ((uint32_t)netif->hwaddr[3] << 24) | ((uint32_t)netif->hwaddr[2] << 16) |
                   ((uint32_t)netif->hwaddr[1] << 8)  | (uint32_t)netif->hwaddr[0];
    ETH->MACA0HR = ((uint32_t)netif->hwaddr[5] << 8)  | (uint32_t)netif->hwaddr[4];

    ethernetif_is_link_up();
    return ERR_OK;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    ETH_DMADescTypeDef *desc = &s_dma_tx_desc[s_tx_desc_idx];

    /* Wait if buffer owned by DMA */
    if (desc->DESC3 & 0x80000000U) {
        return ERR_BUF;
    }

    uint8_t *buffer = (uint8_t *)desc->DESC0;
    pbuf_copy_partial(p, buffer, p->tot_len, 0);

    desc->DESC2 = (uint32_t)p->tot_len;
    /* Set First & Last descriptor, OWN bit */
    desc->DESC3 = 0x80000000U | (1U << 28) | (1U << 29);

    /* Advance descriptor index */
    s_tx_desc_idx = (s_tx_desc_idx + 1) % ETH_TX_DESC_CNT;

    /* Issue poll demand to DMA */
    ETH->DMACTDTPR = (uint32_t)&s_dma_tx_desc[s_tx_desc_idx];
    return ERR_OK;
}

void ethernetif_input(struct netif *netif)
{
    ETH_DMADescTypeDef *desc = &s_dma_rx_desc[s_rx_desc_idx];

    /* Check if DMA has given ownership back to CPU (OWN bit == 0) */
    if ((desc->DESC3 & 0x80000000U) == 0) {
        uint16_t len = (desc->DESC3 & 0x7FFF); /* Packet length */
        if (len > 0 && len <= ETH_RX_BUFFER_SIZE) {
            struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
            if (p != NULL) {
                pbuf_take(p, (const void *)desc->DESC0, len);
                if (netif->input(p, netif) != ERR_OK) {
                    pbuf_free(p);
                }
            }
        }

        /* Return descriptor to DMA */
        desc->DESC3 = 0x80000000U | (1U << 24);
        s_rx_desc_idx = (s_rx_desc_idx + 1) % ETH_RX_DESC_CNT;
        ETH->DMACRDTPR = (uint32_t)&s_dma_rx_desc[s_rx_desc_idx]; /* Poll demand */
    }
}

err_t ethernetif_init(struct netif *netif)
{
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    return low_level_init(netif);
}
