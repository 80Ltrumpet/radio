#include "spi.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace {

void wait_for_spif() {
  while (!(SPSR & _BV(SPIF)))
    ;
}

}  // namespace

void Spi::Transaction::read(void* buffer, uint8_t length) const {
  auto bytes{reinterpret_cast<uint8_t*>(buffer)};
  while (length--) {
    SPDR = 0;
    wait_for_spif();
    *bytes++ = SPDR;
  }
}

void Spi::Transaction::write(const void* buffer, uint8_t length) const {
  auto bytes{reinterpret_cast<const uint8_t*>(buffer)};
  while (length--) {
    SPDR = *bytes++;
    wait_for_spif();
  }
}

void Spi::Transaction::rw(void* rbuf, const void* wbuf, uint8_t length) const {
  auto rbytes{reinterpret_cast<uint8_t*>(rbuf)};
  auto wbytes{reinterpret_cast<const uint8_t*>(wbuf)};
  while (length--) {
    SPDR = *wbytes++;
    wait_for_spif();
    *rbytes++ = SPDR;
  }
}

void Spi::Init() {
  // MOSI and SCK should both be outputs.
  // SS (PB0) is floating, so make sure it is internally pulled up to prevent
  // a spurious slave select from occurring.
  DDRB = (DDRB | _BV(DDB2) | _BV(DDB1)) & ~_BV(DDB0);
  PORTB |= _BV(PORTB0);

  // Master SPI in mode 0 at 4 MHz
  SPCR = _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);
}