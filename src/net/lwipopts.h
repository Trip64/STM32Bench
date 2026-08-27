/* lwIP configuration options for STM32Benchmark bare-metal server */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (32 * 1024)

#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_RAW_PCB                4
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                16

#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536

#define LWIP_STATS                      0

#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0
#define LWIP_DHCP                       0
#define LWIP_AUTOIP                     0
#define LWIP_SNMP                       0
#define LWIP_IGMP                       0
#define LWIP_DNS                        0
#define LWIP_UDP                        0
#define LWIP_TCP                        1

#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                16

#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_ICMP             1

/* HTTPD configuration */
#define LWIP_HTTPD                      1
#define HTTPD_USE_CUSTOM_FSDATA         1
#define HTTPD_FSDATA_FILE               "fsdata.c"
#define LWIP_HTTPD_SSI                  1
#define LWIP_HTTPD_CGI                  1
#define LWIP_HTTPD_SUPPORT_POST         0
#define LWIP_HTTPD_DYNAMIC_HEADERS      0
#define LWIP_HTTPD_REQ_BUFSIZE          2048
#define LWIP_HTTPD_MAX_REQ_LENGTH       2048
#define LWIP_HTTPD_MAX_TAG_NAME_LEN     32
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN   8192

#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1

#endif /* LWIPOPTS_H */
