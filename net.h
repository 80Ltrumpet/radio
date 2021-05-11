#pragma once

#include <stddef.h>
#include <stdint.h>

/* AVR microcontrollers are little-endian, so these operations are two-way */
#define htons(x) (((x) >> 8) | ((x) << 8))
#define ntohs(x) htons(x)

#define htonl(x) (((x) >> 24) | (((x) >> 8) & 0xff00) | \
                 (((x) << 8) & 0xff0000) | ((x) << 24))
#define ntohl(x) htonl(x)

#define HWADDR_LEN 6

extern const uint8_t hwaddr[HWADDR_LEN];

#define NET_BCAST_ADDR     0xffffffff
#define NET_SOCK_INVALID   0xff

typedef struct {
   uint32_t udp_saddr;
   uint16_t udp_sport;
   uint16_t udp_datasize;
} udp_header_t;

uint8_t net_sock_open_udp(uint16_t hport);
void net_sock_close(uint8_t sock);

size_t net_send(uint8_t sock, uint32_t haddr, uint16_t hport, void *data,
      size_t datalen);
size_t net_recv(uint8_t sock, udp_header_t *header, void *buf, size_t bufsize);
