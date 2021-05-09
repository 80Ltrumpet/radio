#include <avr/io.h>

#include "spi.h"
#include "usart.h"

void spi_init(void) {
   /* Set SS as high output */
   PORTB |= _BV(PB0);
   DDRB |= _BV(DDB0);
   /* Enable SPI as master, mode 0 */
   SPCR = _BV(SPE) | _BV(MSTR);
   /* SCK frequency is FOSC/2 */
   SPSR = _BV(SPI2X);
   /* Set SCK and MOSI to outputs. MISO is an input by configuration. */
   DDRB |= _BV(PB1) | _BV(PB2);
}

uint8_t spi_rw(uint8_t data) {
   SPDR = data;
   while (!(SPSR & _BV(SPIF)))
      ;
   return SPDR;
}
