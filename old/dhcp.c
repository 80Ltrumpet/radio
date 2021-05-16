#include "dhcp.h"
#include "w5100.h"
#include "net.h"

#include "printf.h"
#include "string.h"
#include "timer.h"

#define DHCP_OP_BOOTREQUEST   1
#define DHCP_OP_BOOTREPLY     2

#define DHCP_FLAGS_BROADCAST  0x8000

#define DHCP_SERVER_PORT      67
#define DHCP_CLIENT_PORT      68

typedef struct {
   uint8_t  op;
   uint8_t  htype;      // Hardware address type (1 = 10Mb Ethernet)
   uint8_t  hlen;       // Hardware address length (6 for 10Mb Ethernet)
   uint8_t  hops;       // Client sets to zero (optionally used by relay agents)
   uint32_t xid;        // Transaction ID (random number chosen by client)
   uint16_t secs;       // Seconds since client began address acquisition
   uint16_t flags;
   uint32_t ciaddr;     // Client IP address (only filled in in certain cases)
   uint32_t yiaddr;     // "Your" IP address (given by server)
   uint32_t siaddr;     // IP address of next bootstrap server (given by server)
   uint32_t giaddr;     // Relay agent IP address (given by server)
   uint8_t  chaddr[16]; // Client hardware address
   char     sname[64];  // Server host name (or options)
   char     file[128];  // Client boot file path (or options)
   uint8_t  options[0]; // Optional parameters field (variable length)
} dhcp_hdr_t;

typedef struct {
   uint8_t code;
   uint8_t len;
   uint8_t value[0];
} dhcp_opt_t;

uint8_t dhcp_options_magic[] = { 99, 130, 83, 99 };

enum {
   DHCP_OPTION_PAD = 0,
   DHCP_OPTION_SUBNET_MASK,
   DHCP_OPTION_REQUESTED_IP_ADDRESS = 50,
   DHCP_OPTION_IP_ADDRESS_LEASE_TIME,
   DHCP_OPTION_OVERLOAD,
   DHCP_OPTION_MESSAGE_TYPE,
   DHCP_OPTION_SERVER_IDENTIFIER,
   DHCP_OPTION_PARAMETER_REQUEST_LIST,
   DHCP_OPTION_MESSAGE,
   DHCP_OPTION_MAXIMUM_MESSAGE_SIZE,
   DHCP_OPTION_RENEWAL_TIME_VALUE,
   DHCP_OPTION_REBINDING_TIME_VALUE,
   DHCP_OPTION_VENDOR_CLASS_IDENTIFIER,
   DHCP_OPTION_CLIENT_IDENTIFIER,
   DHCP_OPTION_TFTP_SERVER_NAME = 66,
   DHCP_OPTION_BOOTFILE_NAME,
   DHCP_OPTION_END = 255
};

enum {
   DHCP_OVERLOAD_FILE = 1,
   DHCP_OVERLOAD_SNAME,
   DHCP_OVERLOAD_BOTH,
};

enum {
   DHCPDISCOVER = 1,
   DHCPOFFER,
   DHCPREQUEST,
   DHCPDECLINE,
   DHCPACK,
   DHCPNAK,
   DHCPRELEASE,
};

typedef enum {
   DHCP_STATE_STARTUP,
   DHCP_STATE_INIT,
   DHCP_STATE_SELECTING,
   DHCP_STATE_REQUESTING,
   DHCP_STATE_BOUND,
   DHCP_STATE_RENEWING,
   DHCP_STATE_REBINDING,
   DHCP_STATE_ERROR // Keep this last
} dhcp_state_t;

static const char *dhcp_state_names[] = {
   "STARTUP",
   "INIT",
   "SELECTING",
   "REQUESTING",
   "BOUND",
   "RENEWING",
   "REBINDING",
   "ERROR"
};

static dhcp_state_t dhcp_state = DHCP_STATE_STARTUP;

#define DHCP_MSG_BUF_SIZE 512
static uint8_t dhcp_msg_buf[DHCP_MSG_BUF_SIZE];

static uint32_t dhcp_xid;
static uint8_t dhcp_sock = NET_SOCK_INVALID;

static dhcp_state_t dhcp_startup(void) {
   if (dhcp_sock == NET_SOCK_INVALID) {
      dhcp_sock = net_sock_open_udp(DHCP_CLIENT_PORT);
      if (dhcp_sock == NET_SOCK_INVALID) {
         /* TODO Log an error */
         return DHCP_STATE_ERROR;
      }
   }
   return DHCP_STATE_INIT;
}

static dhcp_state_t dhcp_init(void) {
   size_t len, sent;
   dhcp_hdr_t *header = (dhcp_hdr_t *)dhcp_msg_buf;
   dhcp_opt_t *option;

   header->op = DHCP_OP_BOOTREQUEST;
   header->htype = 1;
   header->hlen = HWADDR_LEN;
   header->hops = 0;
   header->xid = dhcp_xid = (uint32_t)ticks; // Should be random enough...
   header->secs = 0;
   header->flags = DHCP_FLAGS_BROADCAST;
   header->ciaddr = 0;
   header->yiaddr = 0;
   header->siaddr = 0;
   header->giaddr = 0;
   memcpy(header->chaddr, hwaddr, HWADDR_LEN);
   bzero(header->sname, sizeof(header->sname));
   bzero(header->file, sizeof(header->file));

   len = sizeof(dhcp_hdr_t);

   memcpy(dhcp_msg_buf + len, dhcp_options_magic, sizeof(dhcp_options_magic));
   len += sizeof(dhcp_options_magic);

   option = (dhcp_opt_t *)(dhcp_msg_buf + len);
   option->code = DHCP_OPTION_MESSAGE_TYPE;
   option->len = 1;
   option->value[0] = DHCPDISCOVER;
   len += sizeof(dhcp_opt_t) + option->len;

   /* Pad to a multiple of four */
   while (len & 3)
      dhcp_msg_buf[len++] = DHCP_OPTION_PAD;

   dhcp_msg_buf[len++] = DHCP_OPTION_END;
   sent = net_send(dhcp_sock, NET_BCAST_ADDR, DHCP_SERVER_PORT, dhcp_msg_buf,
         len);

   /* TODO Retry here (some limited number of times) before erroring out */
   if (sent < len)
      return DHCP_STATE_ERROR;

   /* TODO Record the start time of the requested lease (lease starts at the
    * time the discover is sent). */

   return DHCP_STATE_SELECTING;
}

/* TODO STUB */
static dhcp_state_t dhcp_selecting(void) {
   udp_header_t udp = { 0 };
   size_t recd;

   while ((recd = net_recv(dhcp_sock, &udp, dhcp_msg_buf,
               DHCP_MSG_BUF_SIZE)) > 0) {
      dhcp_hdr_t *hdr;
      dhcp_opt_t *opt;

      /* XXX This seems dumb */
      if (recd < sizeof(dhcp_hdr_t))
         continue;
      hdr = (dhcp_hdr_t *)dhcp_msg_buf;
      recd -= sizeof(dhcp_hdr_t);

      if (recd <= sizeof(dhcp_options_magic))
         continue;
      /* XXX No. No? */
      opt = (dhcp_opt_t *)((uint8_t *)hdr + sizeof(hdr) +
            sizeof(dhcp_options_magic));
      recd -= sizeof(dhcp_options_magic);

      if (hdr->op == DHCP_OP_BOOTREPLY &&
          hdr->xid == dhcp_xid &&
          opt->code == DHCP_OPTION_MESSAGE_TYPE &&
          opt->value[0] == DHCPOFFER) {
         /* TODO Broadcast a DHCPREQUEST specifying the address of the server */
         return DHCP_STATE_REQUESTING;
      }
   }

   return DHCP_STATE_SELECTING;
}

/* TODO STUB */
static dhcp_state_t dhcp_requesting(void) {
   /*
    * Check for packets. If none, return DHCP_STATE_REQUESTING.
    * If the xid of a packet does not match dhcp_xid, discard it.
    * If a packet is a DHCPOFFER, discard it.
    * If a packet is a DHCPNAK, return DHCP_STATE_INIT.
    * If a packet is a DHCPACK, set the IP address of the interface, record the
    * leasing server, set timers, broadcast an ARP request, and return
    * DHCP_STATE_BOUND.
    * Return DHCP_STATE_REQUESTING.
    */
   return DHCP_STATE_ERROR;
}

/* TODO STUB */
static dhcp_state_t dhcp_bound(void) {
   /*
    * Discard all packets.
    * If T1 has expired, send a DHCPREQUEST to the leasing server and return
    * DHCP_STATE_RENEWING.
    * Return DHCP_STATE_BOUND.
    */
   return DHCP_STATE_ERROR;
}

/* TODO STUB */
static dhcp_state_t dhcp_renewing(void) {
   /*
    * If a DHCPACK was received from the leasing server, renew the lease,
    * discard all other packets, and return DHCP_STATE_BOUND.
    * If a DHCPNAK was received from the leasing server, expire the lease,
    * discard all other packets, and return DHCP_STATE_INIT. (We no longer have
    * an IP address.)
    * Discard all packets.
    * If T2 has expired, broadcast a DHCPREQUEST and return
    * DHCP_STATE_REBINDING.
    * Return DHCP_STATE_RENEWING.
    */
   return DHCP_STATE_ERROR;
}

/* TODO STUB */
static dhcp_state_t dhcp_rebinding(void) {
   /*
    * If a DHCPACK was received from any server, record the leasing server, set
    * timers, discard all other packtes, and return DHCP_STATE_BOUND.
    * If a DHCPNACK was received, expire the lease, discard all other packets,
    * and return DHCP_STATE_INIT.
    * Discard all packets.
    * If the lease has expired, return DHCP_STATE_INIT.
    * If it has been between 60 seconds and half of the remaining lease time
    * since T2 expired and the last DHCPREQUEST was sent, re-broadcast the
    * DHCPREQUEST.
    * Return DHCP_STATE_REBINDING.
    */
   return DHCP_STATE_ERROR;
}

/* This is a terminal state for which manual intervention is required */
static dhcp_state_t dhcp_error(void) {
   return DHCP_STATE_ERROR;
}

/* DHCP state handler lookup table */
typedef dhcp_state_t (*dhcp_f)(void);
static dhcp_f dhcp_handlers[] = {
   dhcp_startup,
   dhcp_init,
   dhcp_selecting,
   dhcp_requesting,
   dhcp_bound,
   dhcp_renewing,
   dhcp_rebinding,
   dhcp_error
};

/* TODO Add this to the list of scheduled tasks. */
void dhcp_task(void *arg) {
   dhcp_state = dhcp_handlers[dhcp_state]();
}

void dhcp_cmd(int argc, const char *argv[]) {
   if (argc != 2) {
print_help:
      printf("Usage: dhcp status\n"
             "       dhcp restart\n");
      return;
   }

   if (strcmp(argv[1], "status") == 0) {
      printf("DHCP state: %s\n", dhcp_state_names[dhcp_state]);
   } else if (strcmp(argv[1], "restart") == 0) {
      if (dhcp_state == DHCP_STATE_ERROR) {
         dhcp_state = DHCP_STATE_STARTUP;
         printf("DHCP state machine will restart.\n");
      } else {
         printf("DHCP can only be restarted if there was an error.\n");
      }
   } else {
      goto print_help;
   }
}
