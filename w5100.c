#include <avr/io.h>
#include <avr/interrupt.h>

#include "w5100.h"

#include "net.h"
#include "printf.h"
#include "spi.h"
#include "string.h"

#define W5100_SOCK_MEM_UNUSED 0xffff

static const char *w5100_help =
"Usage: w5100 <addr> r [len]\n"
"                    w <data>\n"
"";

void w5100_init(void) {
   /* PB4 is the /SS line for the W5100 on the Ethernet Shield */
   PORTB |= _BV(PB4);   // Pull up PB4 before setting output
   DDRB |= _BV(DDB4);   // PB4 is an output (initially driven high)

   /* Pull up external interrupt line */
   PORTE |= _BV(PE4);
   /* Enable external interrupt for the /INT line from the W5100 */
   EIMSK |= _BV(INT4);

   /* Reset the W5100 */
   w5100_write_byte(W5100_MR, W5100_MR_RST);
   /* Nothing needs to be written to the mode register */
   /* Enable interrupts for IP conflict, dest unreachable, and all sockets */
   w5100_write_byte(W5100_IMR, W5100_IMR_CONFLICT |
                               W5100_IMR_UNREACH  |
                               W5100_IMR_S3_INT   |
                               W5100_IMR_S2_INT   |
                               W5100_IMR_S1_INT   |
                               W5100_IMR_S0_INT);
   /* Leave the retry time as default */
   /* Leave the retry count as default */
   /* Set the hardware address (this was assigned by IEEE) */
   w5100_write(W5100_SHAR, hwaddr, sizeof(hwaddr));
   /*
    * Gateway address, subnet mask, and source IP address will be configured by
    * DHCP
    */
   /* Split socket memory evenly (default) */
}

uint8_t w5100_read_byte(uint16_t addr) {
   uint8_t data;

   /* Assert SS */
   PORTB &= ~_BV(PB4);

   spi_rw(W5100_SPI_READ);
   spi_rw(addr >> 8);
   spi_rw(addr & 0xff);
   data = spi_rw(0);

   /* Deassert SS */
   PORTB |= _BV(PB4);

   return data;
}

void w5100_write_byte(uint16_t addr, uint8_t data) {
   /* Assert SS */
   PORTB &= ~_BV(PB4);

   spi_rw(W5100_SPI_WRITE);
   spi_rw(addr >> 8);
   spi_rw(addr & 0xff);
   spi_rw(data);

   /* Deassert SS */
   PORTB |= _BV(PB4);
}

void w5100_read(uint16_t addr, void *data, size_t len) {
   uint8_t *out = (uint8_t *)data;
   for (; len > 0; len--)
      *out++ = w5100_read_byte(addr++);
}

void w5100_write(uint16_t addr, const void *data, size_t len) {
   uint8_t *in = (uint8_t *)data;
   for (; len > 0; len--)
      w5100_write_byte(addr++, *in++);
}

void w5100_cmd(int argc, const char *argv[]) {
   unsigned int addr;
   uint8_t data;

   if (argc < 3 || argc > 4)
      goto print_help;

   if (strtohex(argv[1], &addr) != 0) {
      printf("Invalid address.\n");
      goto print_help;
   }

   if (strcmp("r", argv[2]) == 0) {
      uint16_t i, len = 1;
      if (argc == 4 && strtohex(argv[3], &len) != 0) {
         printf("Invalid length.\n");
         goto print_help;
      }
      for (i = 0; i < len; i++) {
         w5100_read(addr + i, &data, 1);
         printf(" %02x", data);
         if ((i & 0xf) == 0xf)
            printf("\n");
      }
      if ((i & 0xf) != 0)
         printf("\n");
      return;
   } else if (strcmp("w", argv[2]) == 0) {
      unsigned int in_data;
      if (argc != 4) {
         printf("No data to write.\n");
         goto print_help;
      }
      if (strtohex(argv[3], &in_data) != 0) {
         printf("Invalid data.\n");
         goto print_help;
      }
      data = (uint8_t)in_data;
      w5100_write(addr, &data, 1);
      return;
   }

print_help:
   printf(w5100_help);
}

ISR(INT4_vect) {
   /* TODO figure out what else to do here */
   /* Clear the interrupt */
   uint8_t ir = w5100_read_byte(W5100_IR);
   if (ir & W5100_IR_S0_INT)
      w5100_write_byte(W5100_Sn_IR(0), 1);
   if (ir & W5100_IR_S1_INT)
      w5100_write_byte(W5100_Sn_IR(1), 1);
   if (ir & W5100_IR_S2_INT)
      w5100_write_byte(W5100_Sn_IR(2), 1);
   if (ir & W5100_IR_S3_INT)
      w5100_write_byte(W5100_Sn_IR(3), 1);
   if (ir & W5100_IR_NONSOCK_MASK)
      w5100_write_byte(W5100_IR, W5100_IR_NONSOCK_MASK);
}
