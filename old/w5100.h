#ifndef _W5100_H
#define _W5100_H

#include <stddef.h>
#include <stdint.h>

/*
 * Register Addresses ---------------------------------------------------------
 */

/* Common Registers */

#define W5100_MR        0x0000   // Mode
#define W5100_GAR       0x0001   // Gateway Address (32-bit)
#define W5100_SUBR      0x0005   // Subnet mask Address (32-bit)
#define W5100_SHAR      0x0009   // Source Hardware Address (48-bit)
#define W5100_SIPR      0x000f   // Source IP Address (32-bit)
#define W5100_IR        0x0015   // Interrupt
#define W5100_IMR       0x0016   // Interrupt Mask
#define W5100_RTR       0x0017   // Retry Time (16-bit)
#define W5100_RCR       0x0019   // Retry Count
#define W5100_RMSR      0x001a   // RX Memory Size
#define W5100_TMSR      0x001b   // TX Memory Size
#define W5100_PATR      0x001c   // Authentication Type in PPPoE (16-bit)
#define W5100_PTIMER    0x0028   // PPP LCP Request Timer
#define W5100_PMAGIC    0x0029   // PPP LCP Magic Number
#define W5100_UIPR      0x002a   // Unreachable IP Address (32-bit)
#define W5100_UPORT     0x002e   // Unreachable Port (16-bit)

/* Socket Registers */

#define W5100_Sn(n, off)     ((((n) + 4) << 8) | (off))

#define W5100_Sn_MR(n)        W5100_Sn(n, 0x0000) // Socket Mode
#define W5100_Sn_CR(n)        W5100_Sn(n, 0x0001) // Socket Command
#define W5100_Sn_IR(n)        W5100_Sn(n, 0x0002) // Socket Interrupt
#define W5100_Sn_SR(n)        W5100_Sn(n, 0x0003) // Socket Status
#define W5100_Sn_PORT(n)      W5100_Sn(n, 0x0004) // Socket Source Port
#define W5100_Sn_DHAR(n)      W5100_Sn(n, 0x0006) // Socket Dest HW Address
#define W5100_Sn_DIPR(n)      W5100_Sn(n, 0x000c) // Socket Dest IP Address
#define W5100_Sn_DPORT(n)     W5100_Sn(n, 0x0010) // Socket Destination Port
#define W5100_Sn_MSSR(n)      W5100_Sn(n, 0x0012) // Socket Max Seg Size
#define W5100_Sn_PROTO(n)     W5100_Sn(n, 0x0014) // Socket Proto in IP Raw Mode
#define W5100_Sn_TOS(n)       W5100_Sn(n, 0x0015) // Socket IP TOS
#define W5100_Sn_TTL(n)       W5100_Sn(n, 0x0016) // Socket IP TTL
#define W5100_Sn_TX_FSR(n)    W5100_Sn(n, 0x0020) // Socket TX Free Size
#define W5100_Sn_TX_RD(n)     W5100_Sn(n, 0x0022) // Socket TX Read Pointer
#define W5100_Sn_TX_WR(n)     W5100_Sn(n, 0x0024) // Socket TX Write Pointer
#define W5100_Sn_RX_RSR(n)    W5100_Sn(n, 0x0026) // Socket RX Received Size
#define W5100_Sn_RX_RD(n)     W5100_Sn(n, 0x0028) // Socket RX Read Pointer

/*
 * Register Descriptions ------------------------------------------------------
 */

/* Common Registers */

/* Mode Register */
#define W5100_MR_RST          0x80     // S/W Reset
#define W5100_MR_PB           0x10     // Ping Block Mode
#define W5100_MR_PPPoE        0x08     // PPPoE Mode
#define W5100_MR_AI           0x02     // Address Auto-Inc in Indirect Bus I/F
#define W5100_MR_IND          0x01     // Indirect Bus I/F Mode

/* Interrupt Register */
#define W5100_IR_CONFLICT     0x80     // IP Conflict
#define W5100_IR_UNREACH      0x40     // Destination Unreachable
#define W5100_IR_PPPoE        0x20     // PPPoE Connection Close
#define W5100_IR_S3_INT       0x08     // Occurrence of Socket 3 Interrupt
#define W5100_IR_S2_INT       0x04     // Occurrence of Socket 2 Interrupt
#define W5100_IR_S1_INT       0x02     // Occurrence of Socket 1 Interrupt
#define W5100_IR_S0_INT       0x01     // Occurrence of Socket 0 Interrupt

#define W5100_IR_NONSOCK_MASK 0xe0
#define W5100_IR_SOCK_MASK    0x0f

/* Interrupt Mask Register */
#define W5100_IMR_CONFLICT    0x80     // IP Conflict Enable
#define W5100_IMR_UNREACH     0x40     // Destination Unreachable Enable
#define W5100_IMR_PPPoE       0x20     // PPPoE Connection Close Enable
#define W5100_IMR_S3_INT      0x08     // Socket 3 Interrupt Enable
#define W5100_IMR_S2_INT      0x04     // Socket 2 Interrupt Enable
#define W5100_IMR_S1_INT      0x02     // Socket 1 Interrupt Enable
#define W5100_IMR_S0_INT      0x01     // Socket 0 Interrupt Enable

/* Retry Time-value Register */
#define W5100_RTR_US          100      // 100 microseconds per step

/* RX/TX Memory Size Register */
#define W5100_xMSR_Sn_R(r, n) (((r) >> (2 * (n))) & 0x03) // Read xMSR for Sn
#define W5100_xMSR_Sn_W(v, n) (((v) & 0x03) << (2 * (n))) // Write xMSR for Sn
#define W5100_xMSR_1KB        0        // 1 KB Memory Size
#define W5100_xMSR_2KB        1        // 2 KB Memory Size
#define W5100_xMSR_4KB        2        // 4 KB Memory Size
#define W5100_xMSR_8KB        3        // 8 KB Memory Size

/* Authentication Type in PPPoE Mode Register */
#define W5100_PATR_PAP        0xc028   // PAP Authentication
#define W5100_PATR_CHAP       0xc223   // CHAP Authentication

/* PPP Link Control Protocol Request Timer Register */
#define W5100_PTIMER_MS       25       // 25 milliseconds per step

/* Socket Registers */

/* Socket n Mode Register */
#define W5100_Sn_MR_MULTI     0x80     // Multicasting
#define W5100_Sn_MR_NDMC      0x20     // Use No Delayed ACK/Multicast
#define W5100_Sn_MR_PROTO     0x0f     // Protocol
#define W5100_Sn_MR_PROTO_CLOSED    0  // Closed
#define W5100_Sn_MR_PROTO_TCP       1  // TCP
#define W5100_Sn_MR_PROTO_UDP       2  // UDP
#define W5100_Sn_MR_PROTO_IPRAW     3  // IPRAW
#define W5100_Sn_MR_PROTO_MACRAW    4  // MACRAW
#define W5100_Sn_MR_PROTO_PPPoE     5  // PPPoE

/* Socket n Command Register */
#define W5100_Sn_CR_OPEN      0x01     // OPEN
#define W5100_Sn_CR_LISTEN    0x02     // LISTEN
#define W5100_Sn_CR_CONNECT   0x04     // CONNECT
#define W5100_Sn_CR_DISCON    0x08     // DISCON
#define W5100_Sn_CR_CLOSE     0x10     // CLOSE
#define W5100_Sn_CR_SEND      0x20     // SEND
#define W5100_Sn_CR_SEND_MAC  0x21     // SEND_MAC
#define W5100_Sn_CR_SEND_KEEP 0x22     // SEND_KEEP
#define W5100_Sn_CR_RECV      0x40     // RECV

/* Socket n Interrupt Register */
#define W5100_Sn_IR_SEND_OK   0x10     // Send operation is completed
#define W5100_Sn_IR_TIMEOUT   0x08     // Timeout occurred
#define W5100_Sn_IR_RECV      0x04     // Received data
#define W5100_Sn_IR_DISCON    0x02     // Connection terminated
#define W5100_Sn_IR_CON       0x01     // Connection established

/* Socket n Status Register */
/* States */
#define W5100_Sn_SR_SOCK_CLOSED        0x00
#define W5100_Sn_SR_SOCK_INIT          0x13
#define W5100_Sn_SR_SOCK_LISTEN        0x14
#define W5100_Sn_SR_SOCK_ESTABLISHED   0x17
#define W5100_Sn_SR_SOCK_CLOSE_WAIT    0x1c
#define W5100_Sn_SR_SOCK_UDP           0x22
#define W5100_Sn_SR_SOCK_IPRAW         0x32
#define W5100_Sn_SR_SOCK_MACRAW        0x42
#define W5100_Sn_SR_SOCK_PPPOE         0x5f
/* Transitions */
#define W5100_Sn_SR_SOCK_SYNSENT       0x15
#define W5100_Sn_SR_SOCK_SYNRECV       0x16
#define W5100_Sn_SR_SOCK_FIN_WAIT      0x18
#define W5100_Sn_SR_SOCK_CLOSING       0x1a
#define W5100_Sn_SR_SOCK_TIME_WAIT     0x1b
#define W5100_Sn_SR_SOCK_LAST_ACK      0x1d
#define W5100_Sn_SR_SOCK_ARP(r)        ((r) == 0x11 || \
                                        (r) == 0x21 || \
                                        (r) == 0x31)

/*
 * Memory Address Space -------------------------------------------------------
 */

#define W5100_TX_BASE         0x4000
#define W5100_RX_BASE         0x6000
#define W5100_SOCK_MEM_SIZE   0x2000

#define W5100_ADDR_LIMIT      (W5100_RX_BASE + W5100_SOCK_MEM_SIZE)

/*
 * SPI ------------------------------------------------------------------------
 */

#define W5100_SPI_WRITE       0xf0
#define W5100_SPI_READ        0x0f

/*
 * API ------------------------------------------------------------------------
 */

#define W5100_NUM_SOCKETS     4

void w5100_init(void);
uint8_t w5100_read_byte(uint16_t addr);
void w5100_write_byte(uint16_t addr, uint8_t data);
void w5100_read(uint16_t addr, void *data, size_t len);
void w5100_write(uint16_t addr, const void *data, size_t len);
void w5100_cmd(int argc, const char *argv[]);

#endif
