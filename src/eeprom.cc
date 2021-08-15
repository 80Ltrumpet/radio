#include "eeprom.h"

#include <avr/io.h>

#include "atomic.h"

namespace {

// Reads a single byte from the EEPROM. Interrupts must be disabled before
// calling this function. This function does not check if the address is out of
// bounds.
uint8_t read_byte(uint16_t addr) {
  while (EECR & _BV(EEPE))
    ;
  EEAR = addr;
  EECR |= _BV(EERE);
  return EEDR;
}

}

namespace Eeprom {

void Erase(const Datum& datum) {
  for (uint16_t i{}; datum.address + i <= E2END && i < datum.length; ++i) {
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = datum.address + i;
    EECR = (EECR & ~_BV(EEPM1)) | _BV(EEPM0) | _BV(EEMPE);
    EECR |= _BV(EEPE);
  }
}

// Returns the number of bytes actually read.
uint16_t Read(const Datum& datum, void* output) {
  auto addr{datum.address};
  auto bytes{reinterpret_cast<uint8_t*>(output)};
  uint16_t i{};
  for (; addr <= E2END && i < datum.length; ++addr, ++i) {
    AtomicLock lock{};
    bytes[i] = read_byte(addr);
  }

  return i;
}

// Writes the EEPROM if the data in the input buffer is different from what is
// already stored.
void Update(const Datum& datum, const void* input) {
  auto addr{datum.address};
  auto bytes{reinterpret_cast<const uint8_t*>(input)};
  for (uint16_t i{}; addr <= E2END && i < datum.length; ++addr, ++i) {
    auto byte{read_byte(addr)};
    if (bytes[i] == byte) continue;
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = addr;
    EEDR = bytes[i];
    EECR = (EECR & ~(_BV(EEPM1) | _BV(EEPM0))) | _BV(EEMPE);
    EECR |= _BV(EEPE);
  }
}

}  // namespace Eeprom