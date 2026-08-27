/* Bare-metal USB 2.0 Full-Speed CDC ACM implementation for STM32F072 */

#include "usb_f0_fs.h"
#include "stm32f072xb.h"
#include <string.h>

#define PMA_BASE_ADDR           0x40006000UL
#define PMA_WORD(offset)        (*(volatile uint16_t *)(PMA_BASE_ADDR + (offset)))

/* PMA Buffer Offsets (Bytes) */
#define BTABLE_OFFSET           0x0000
#define EP0_TX_ADDR             0x0040
#define EP0_RX_ADDR             0x0080
#define EP1_TX_ADDR             0x00C0
#define EP2_RX_ADDR             0x0100
#define EP3_TX_ADDR             0x0140

#define EP_PACKET_SIZE          64

/* USB Standard Descriptors */
static const uint8_t s_device_descriptor[] = {
    0x12,                       /* bLength */
    0x01,                       /* bDescriptorType = Device */
    0x00, 0x02,                 /* bcdUSB = 2.00 */
    0x02,                       /* bDeviceClass = CDC */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    0x40,                       /* bMaxPacketSize0 = 64 */
    0x83, 0x04,                 /* idVendor = 0x0483 (STMicroelectronics) */
    0x40, 0x57,                 /* idProduct = 0x5740 (Virtual COM Port) */
    0x00, 0x02,                 /* bcdDevice = 2.00 */
    0x01,                       /* iManufacturer = String 1 */
    0x02,                       /* iProduct = String 2 */
    0x03,                       /* iSerialNumber = String 3 */
    0x01                        /* bNumConfigurations = 1 */
};

static const uint8_t s_config_descriptor[] = {
    /* Configuration Descriptor */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0xC0, 0x32,

    /* Interface 0: CDC Communication Class */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* Header Functional Descriptor */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* Call Management Functional Descriptor */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* ACM Functional Descriptor */
    0x04, 0x24, 0x02, 0x02,
    /* Union Functional Descriptor */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* Endpoint 3: Notification (Interrupt IN) */
    0x07, 0x05, 0x83, 0x03, 0x10, 0x00, 0x10,

    /* Interface 1: CDC Data Class */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Endpoint 2: Bulk OUT */
    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
    /* Endpoint 1: Bulk IN */
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
};

static const uint8_t s_lang_descriptor[] = { 0x04, 0x03, 0x09, 0x04 }; /* English US */

/* UTF-16 String Descriptors */
static const uint8_t s_str_mfr[] = {
    24, 0x03,
    'S',0,'T',0,'M',0,'i',0,'c',0,'r',0,'o',0,'e',0,'l',0,'e',0,'c',0
};

static const uint8_t s_str_prod[] = {
    48, 0x03,
    'S',0,'T',0,'M',0,'3',0,'2',0,' ',0,'B',0,'e',0,'n',0,'c',0,'h',0,
    'm',0,'a',0,'r',0,'k',0,' ',0,'C',0,'D',0,'C',0
};

static const uint8_t s_str_sn[] = {
    32, 0x03,
    'F',0,'0',0,'7',0,'2',0,'-',0,'B',0,'E',0,'N',0,'C',0,'H',0,'0',0,'1',0
};

/* CDC Line Coding: 115200 8N1 */
static uint8_t s_line_coding[7] = {
    0x00, 0xC2, 0x01, 0x00,     /* 115200 baud */
    0x00,                       /* 1 stop bit */
    0x00,                       /* No parity */
    0x08                        /* 8 data bits */
};

/* State Variables */
static volatile bool s_configured = false;
static uint8_t s_dev_address = 0;
static bool s_set_address_pending = false;

/* Data Stage Pointer */
static const uint8_t *s_ep0_data_in = NULL;
static uint16_t s_ep0_data_in_len = 0;

/* RX Ring Buffer */
#define RX_BUF_SIZE 256
static volatile uint8_t s_rx_buf[RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* TX State */
static volatile bool s_ep1_busy = false;

/* Helper Macros for EPnR */
static inline volatile uint16_t* ep_reg(uint8_t ep)
{
    return (volatile uint16_t*)(USB_BASE + ep * 4);
}

static void set_stat_tx(uint8_t ep, uint16_t stat)
{
    volatile uint16_t *r = ep_reg(ep);
    uint16_t val = *r;
    *r = ((val ^ stat) & USB_EPTX_STAT) | (val & USB_EPREG_MASK);
}

static void set_stat_rx(uint8_t ep, uint16_t stat)
{
    volatile uint16_t *r = ep_reg(ep);
    uint16_t val = *r;
    *r = ((val ^ stat) & USB_EPRX_STAT) | (val & USB_EPREG_MASK);
}

/* PMA Read / Write (Direct 16-bit halfword access on STM32F0) */
static void pma_write(uint16_t pma_offset, const uint8_t *src, uint16_t len)
{
    volatile uint16_t *dst = (volatile uint16_t *)(PMA_BASE_ADDR + pma_offset);
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t val = src[i];
        if (i + 1 < len) val |= (src[i + 1] << 8);
        *dst++ = val;
    }
}

static void pma_read(uint16_t pma_offset, uint8_t *dst, uint16_t len)
{
    const volatile uint16_t *src = (const volatile uint16_t *)(PMA_BASE_ADDR + pma_offset);
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t val = *src++;
        dst[i] = val & 0xFF;
        if (i + 1 < len) dst[i + 1] = (val >> 8) & 0xFF;
    }
}

/* Set BTABLE entry */
static void set_btable(uint8_t ep, uint16_t tx_addr, uint16_t tx_cnt, uint16_t rx_addr, uint16_t rx_cnt_blocks)
{
    PMA_WORD(BTABLE_OFFSET + ep * 8 + 0) = tx_addr;
    PMA_WORD(BTABLE_OFFSET + ep * 8 + 2) = tx_cnt;
    PMA_WORD(BTABLE_OFFSET + ep * 8 + 4) = rx_addr;
    PMA_WORD(BTABLE_OFFSET + ep * 8 + 6) = rx_cnt_blocks;
}

/* Send packet on EP0 */
static void ep0_send(const uint8_t *data, uint16_t len)
{
    uint16_t pkt_len = (len > EP_PACKET_SIZE) ? EP_PACKET_SIZE : len;
    if (data && pkt_len > 0) {
        pma_write(EP0_TX_ADDR, data, pkt_len);
        s_ep0_data_in = data + pkt_len;
        s_ep0_data_in_len = len - pkt_len;
    } else {
        s_ep0_data_in = NULL;
        s_ep0_data_in_len = 0;
    }
    PMA_WORD(BTABLE_OFFSET + 0 * 8 + 2) = pkt_len; /* COUNT0_TX */
    set_stat_tx(0, USB_EP_TX_VALID);
}

/* Stall EP0 */
static void ep0_stall(void)
{
    set_stat_tx(0, USB_EP_TX_STALL);
    set_stat_rx(0, USB_EP_RX_STALL);
}

/* USB Reset sequence */
static void usb_reset(void)
{
    s_configured = false;
    s_dev_address = 0;
    s_set_address_pending = false;
    s_ep1_busy = false;

    USB->BTABLE = BTABLE_OFFSET;

    /* EP0: Control 64 bytes. RX block size = 32, 2 blocks = 64 bytes (0x8400) */
    set_btable(0, EP0_TX_ADDR, 0, EP0_RX_ADDR, (1U << 15) | (1U << 10));
    *ep_reg(0) = USB_EP_CONTROL | USB_EP_TX_NAK | USB_EP_RX_VALID;

    /* EP1: Bulk IN (64 bytes) - Device to Host */
    set_btable(1, EP1_TX_ADDR, 0, 0, 0);
    *ep_reg(1) = USB_EP_BULK | 1 | USB_EP_TX_NAK | USB_EP_RX_DIS;

    /* EP2: Bulk OUT (64 bytes) - Host to Device */
    set_btable(2, 0, 0, EP2_RX_ADDR, (1U << 15) | (1U << 10));
    *ep_reg(2) = USB_EP_BULK | 2 | USB_EP_TX_DIS | USB_EP_RX_VALID;

    /* EP3: Interrupt IN (16 bytes) */
    set_btable(3, EP3_TX_ADDR, 0, 0, 0);
    *ep_reg(3) = USB_EP_INTERRUPT | 3 | USB_EP_TX_NAK | USB_EP_RX_DIS;

    USB->DADDR = USB_DADDR_EF | 0; /* Enable function, address 0 */
}

/* EP0 Setup Packet Processing */
static void ep0_handle_setup(void)
{
    uint8_t setup_buf[8];
    pma_read(EP0_RX_ADDR, setup_buf, 8);

    uint8_t  bmRequestType = setup_buf[0];
    uint8_t  bRequest      = setup_buf[1];
    uint16_t wValue        = setup_buf[2] | (setup_buf[3] << 8);
    uint16_t wIndex        = setup_buf[4] | (setup_buf[5] << 8);
    uint16_t wLength       = setup_buf[6] | (setup_buf[7] << 8);

    (void)wIndex;

    /* Standard Device Requests */
    if ((bmRequestType & 0x60) == 0x00) {
        switch (bRequest) {
        case 0x06: /* GET_DESCRIPTOR */
            switch (wValue >> 8) {
            case 0x01: /* Device Descriptor */
                ep0_send(s_device_descriptor, (wLength < sizeof(s_device_descriptor)) ? wLength : sizeof(s_device_descriptor));
                return;
            case 0x02: /* Configuration Descriptor */
                ep0_send(s_config_descriptor, (wLength < sizeof(s_config_descriptor)) ? wLength : sizeof(s_config_descriptor));
                return;
            case 0x03: /* String Descriptors */
                switch (wValue & 0xFF) {
                case 0: ep0_send(s_lang_descriptor, sizeof(s_lang_descriptor)); return;
                case 1: ep0_send(s_str_mfr, sizeof(s_str_mfr)); return;
                case 2: ep0_send(s_str_prod, sizeof(s_str_prod)); return;
                case 3: ep0_send(s_str_sn, sizeof(s_str_sn)); return;
                default: ep0_stall(); return;
                }
            default:
                ep0_stall();
                return;
            }

        case 0x05: /* SET_ADDRESS */
            s_dev_address = wValue & 0x7F;
            s_set_address_pending = true;
            ep0_send(NULL, 0); /* ZLP ACK */
            return;

        case 0x09: /* SET_CONFIGURATION */
            s_configured = (wValue != 0);
            ep0_send(NULL, 0); /* ZLP ACK */
            return;

        case 0x00: /* GET_STATUS */
        {
            static const uint8_t status[2] = {0, 0};
            ep0_send(status, 2);
            return;
        }

        default:
            ep0_stall();
            return;
        }
    }
    /* CDC Class Specific Requests (0x21) */
    else if ((bmRequestType & 0x60) == 0x20) {
        switch (bRequest) {
        case 0x20: /* SET_LINE_CODING */
            /* Data will arrive in EP0 OUT stage */
            set_stat_rx(0, USB_EP_RX_VALID);
            return;

        case 0x21: /* GET_LINE_CODING */
            ep0_send(s_line_coding, sizeof(s_line_coding));
            return;

        case 0x22: /* SET_CONTROL_LINE_STATE */
            ep0_send(NULL, 0); /* ZLP ACK */
            return;

        default:
            ep0_stall();
            return;
        }
    }

    ep0_stall();
}

void USB_F0_Init(void)
{
    /* 1. Clocks: Enable HSI48 and CRS for crystal-less USB operation */
    RCC->CR2 |= RCC_CR2_HSI48ON;
    while (!(RCC->CR2 & RCC_CR2_HSI48RDY)) {}

    RCC->APB1ENR |= RCC_APB1ENR_CRSEN;
    CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    /* Select HSI48 as USB clock source */
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW; /* HSI48 is 0 */

    /* Enable USB peripheral clock */
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    __DSB();

    /* 2. Configure PA11 (DM) and PA12 (DP) for USB */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~((3U << (11 * 2)) | (3U << (12 * 2)));
    GPIOA->MODER |=  ((2U << (11 * 2)) | (2U << (12 * 2))); /* AF mode */
    GPIOA->AFR[1] &= ~((0xFU << ((11 - 8) * 4)) | (0xFU << ((12 - 8) * 4))); /* AF0 */

    /* 3. Enable internal 1.5 kΩ D+ pull-up resistor */
    USB->BCDR |= USB_BCDR_DPPU;

    /* 4. Reset USB macro */
    USB->CNTR = USB_CNTR_FRES;
    for (volatile int i = 0; i < 1000; i++) {}
    USB->CNTR = 0;
    USB->ISTR = 0;

    /* Enable interrupts (polled) */
    USB->CNTR = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_WKUPM | USB_CNTR_SUSPM;
}

void USB_F0_Poll(void)
{
    uint16_t istr = USB->ISTR;

    /* USB Reset */
    if (istr & USB_ISTR_RESET) {
        usb_reset();
        USB->ISTR = (uint16_t)~USB_ISTR_RESET;
        return;
    }

    /* Correct Transfer */
    while (USB->ISTR & USB_ISTR_CTR) {
        istr = USB->ISTR;
        uint8_t ep = istr & USB_ISTR_EP_ID;
        volatile uint16_t *epr = ep_reg(ep);
        uint16_t ep_val = *epr;

        /* Endpoint 0 (Control) */
        if (ep == 0) {
            if (ep_val & USB_EP_CTR_RX) {
                if (ep_val & USB_EP_SETUP) {
                    ep0_handle_setup();
                } else {
                    /* Data OUT stage (e.g. SET_LINE_CODING) */
                    uint16_t rx_cnt = PMA_WORD(BTABLE_OFFSET + 0 * 8 + 6) & 0x3FF;
                    if (rx_cnt >= 7) {
                        pma_read(EP0_RX_ADDR, s_line_coding, 7);
                    }
                    ep0_send(NULL, 0); /* ZLP status */
                }
                *epr = (ep_val & USB_EPREG_MASK) & ~USB_EP_CTR_RX;
                set_stat_rx(0, USB_EP_RX_VALID);
            }

            if (ep_val & USB_EP_CTR_TX) {
                *epr = (ep_val & USB_EPREG_MASK) & ~USB_EP_CTR_TX;

                if (s_set_address_pending) {
                    s_set_address_pending = false;
                    USB->DADDR = USB_DADDR_EF | s_dev_address;
                } else if (s_ep0_data_in && s_ep0_data_in_len > 0) {
                    ep0_send(s_ep0_data_in, s_ep0_data_in_len);
                } else {
                    set_stat_rx(0, USB_EP_RX_VALID);
                }
            }
        }
        /* Endpoint 1 (CDC Bulk IN - TX to Host) */
        else if (ep == 1) {
            if (ep_val & USB_EP_CTR_TX) {
                *epr = (ep_val & USB_EPREG_MASK) & ~USB_EP_CTR_TX;
                s_ep1_busy = false;
            }
        }
        /* Endpoint 2 (CDC Bulk OUT - RX from Host) */
        else if (ep == 2) {
            if (ep_val & USB_EP_CTR_RX) {
                uint16_t count = PMA_WORD(BTABLE_OFFSET + 2 * 8 + 6) & 0x3FF;
                uint8_t temp[EP_PACKET_SIZE];
                pma_read(EP2_RX_ADDR, temp, count);

                for (uint16_t i = 0; i < count; i++) {
                    uint16_t next = (s_rx_head + 1) % RX_BUF_SIZE;
                    if (next != s_rx_tail) {
                        s_rx_buf[s_rx_head] = temp[i];
                        s_rx_head = next;
                    }
                }

                *epr = (ep_val & USB_EPREG_MASK) & ~USB_EP_CTR_RX;
                set_stat_rx(2, USB_EP_RX_VALID);
            }
        }
        /* Endpoint 3 (CDC Interrupt IN) */
        else if (ep == 3) {
            *epr = (ep_val & USB_EPREG_MASK) & ~USB_EP_CTR_TX;
        }
    }
}

bool USB_F0_IsConfigured(void)
{
    return s_configured;
}

void USB_F0_WriteBytes(const uint8_t *data, size_t len)
{
    if (!s_configured || !data || len == 0) return;

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > EP_PACKET_SIZE) chunk = EP_PACKET_SIZE;

        /* Wait for previous packet */
        uint32_t timeout = 100000;
        while (s_ep1_busy && --timeout) {
            USB_F0_Poll();
        }
        if (timeout == 0) {
            s_ep1_busy = false;
            break;
        }

        pma_write(EP1_TX_ADDR, data + offset, (uint16_t)chunk);
        PMA_WORD(BTABLE_OFFSET + 1 * 8 + 2) = (uint16_t)chunk; /* COUNT1_TX */
        s_ep1_busy = true;
        set_stat_tx(1, USB_EP_TX_VALID);

        offset += chunk;
    }
}

void USB_F0_WriteChar(char c)
{
    USB_F0_WriteBytes((const uint8_t *)&c, 1);
}

void USB_F0_WriteString(const char *str)
{
    if (!str) return;
    USB_F0_WriteBytes((const uint8_t *)str, strlen(str));
}

bool USB_F0_HasChar(void)
{
    return s_rx_head != s_rx_tail;
}

char USB_F0_ReadChar(void)
{
    if (s_rx_head == s_rx_tail) return 0;
    char c = (char)s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % RX_BUF_SIZE;
    return c;
}
