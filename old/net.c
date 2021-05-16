#include <avr/io.h>

#include "net.h"
#include "w5100.h"

const uint8_t hwaddr[HWADDR_LEN] = { 0x90, 0xa2, 0xda, 0x00, 0x47, 0xaf };

static uint8_t net_sock_lock = 0;

static uint16_t sock_base[W5100_NUM_SOCKETS] = {
   0x0000, 0x0800, 0x1000, 0x1800
};

static uint16_t sock_mask[W5100_NUM_SOCKETS] = {
   0x07ff, 0x07ff, 0x07ff, 0x07ff
};

/* Returns the socket ID of the opened socket */
uint8_t net_sock_open_udp(uint16_t hport) {
   uint8_t sock = 0;
   uint16_t nport = htons(hport);

   /* Find a free socket */
   while ((net_sock_lock >> sock) & 1)
      sock++;

   if (sock >= W5100_NUM_SOCKETS)
      return NET_SOCK_INVALID;

   /* Set socket mode */
   w5100_write_byte(W5100_Sn_MR(sock), W5100_Sn_MR_PROTO_UDP);
   /* Set source port */
   w5100_write(W5100_Sn_PORT(sock), &nport, sizeof (nport));
   /* Open the socket */
   w5100_write_byte(W5100_Sn_CR(sock), W5100_Sn_CR_OPEN);
   /* If the socket does not report the requested mode, an error occurred */
   if (w5100_read_byte(W5100_Sn_SR(sock)) != W5100_Sn_MR_PROTO_UDP) {
      net_sock_close(sock);
      return NET_SOCK_INVALID;
   }

   /* Lock the socket, now that it has been successfully opened */
   net_sock_lock |= _BV(sock);
   return sock;
}

/* Closes the given socket, if open */
void net_sock_close(uint8_t sock) {
   if (!(net_sock_lock & _BV(sock)))
      return;
   w5100_write_byte(W5100_Sn_CR(sock), W5100_Sn_CR_CLOSE);
   net_sock_lock &= _BV(sock);
}

static void net_tx(uint8_t sock, uint16_t offset, void *data, size_t datalen) {
   uint16_t addr = (W5100_TX_BASE | sock_base[sock]) + offset;

   if (offset + datalen > sock_mask[sock] + 1) {
      uint16_t first_chunk = sock_mask[sock] + 1 - offset;
      w5100_write(addr, data, first_chunk);
      w5100_write(W5100_TX_BASE | sock_base[sock], data + first_chunk,
            datalen - first_chunk);
   } else {
      w5100_write(addr, data, datalen);
   }
}

/* Returns the number of bytes placed in the socket's TX buffer */
size_t net_send(uint8_t sock, uint32_t haddr, uint16_t hport, void *data,
      size_t datalen) {
   uint16_t free_size = 0;
   uint16_t nport = htons(hport);
   uint32_t naddr = htonl(haddr);
   uint16_t raw_offset = 0, offset;

   /* Get the TX free size */
   w5100_read(W5100_Sn_TX_FSR(sock), &free_size, sizeof (free_size));
   free_size = ntohs(free_size);
   if (free_size == 0)
      return 0;
   if (free_size < datalen)
      datalen = free_size;

   /* Set destination */
   w5100_write(W5100_Sn_DIPR(sock), &naddr, sizeof (naddr));
   w5100_write(W5100_Sn_DPORT(sock), &nport, sizeof (nport));

   w5100_read(W5100_Sn_TX_WR(sock), &raw_offset, sizeof (raw_offset));
   raw_offset = ntohs(raw_offset);
   offset = raw_offset & sock_mask[sock];
   net_tx(sock, offset, data, datalen);
   raw_offset += datalen;
   raw_offset = htons(raw_offset);
   w5100_write(W5100_Sn_TX_WR(sock), &raw_offset, sizeof (raw_offset));
   w5100_write_byte(W5100_Sn_CR(sock), W5100_Sn_CR_SEND);

   return datalen;
}

static uint16_t net_rx(uint8_t sock, uint16_t offset, void *buf, size_t bufsize)
{
   uint16_t addr = (W5100_RX_BASE | sock_base[sock]) + offset;

   if (offset + bufsize > sock_mask[sock] + 1) {
      uint16_t first_chunk = sock_mask[sock] + 1 - offset;
      offset = bufsize - first_chunk;
      w5100_read(addr, buf, first_chunk);
      w5100_read(W5100_RX_BASE | sock_base[sock], buf + first_chunk, offset);
   } else {
      offset += bufsize;
      w5100_read(addr, buf, bufsize);
   }

   return offset;
}

/* Returns the number of bytes received into the specified buffer */
size_t net_recv(uint8_t sock, udp_header_t *header, void *buf, size_t bufsize) {
   uint16_t recv_size = 0;
   uint16_t raw_offset = 0, offset;

   /* Get the RX size */
   w5100_read(W5100_Sn_RX_RSR(sock), &recv_size, sizeof (recv_size));
   recv_size = ntohs(recv_size);
   if (recv_size < sizeof (*header))
      return 0;

   w5100_read(W5100_Sn_RX_RD(sock), &raw_offset, sizeof (raw_offset));
   raw_offset = ntohs(raw_offset);
   offset = raw_offset & sock_mask[sock];
   offset = net_rx(sock, offset, header, sizeof (*header));
   recv_size -= sizeof (*header);

   /* Convert header data to host byte order */
   header->udp_saddr = ntohl(header->udp_saddr);
   header->udp_sport = ntohs(header->udp_sport);
   header->udp_datasize = ntohs(header->udp_datasize);

   /*
    * If the received size were less than the length of the packet data, there
    * may be some corruption in the transaction. In that case, we need to flush
    * the remainder of the receive buffer.
    */
   if (recv_size > header->udp_datasize)
      recv_size = header->udp_datasize;
   /* If the buffer can't fit the entire packet, just truncate it. */
   if (recv_size < bufsize)
      bufsize = recv_size;

   offset = net_rx(sock, offset, buf, bufsize);
   raw_offset += sizeof (*header) + recv_size;
   raw_offset = htons(raw_offset);
   w5100_write(W5100_Sn_RX_RD(sock), &raw_offset, sizeof (raw_offset));
   w5100_write_byte(W5100_Sn_CR(sock), W5100_Sn_CR_SEND);

   return bufsize;
}
