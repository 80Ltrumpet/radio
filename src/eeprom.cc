#include "eeprom.h"

#include <avr/io.h>

#include "atomic.h"

namespace Eeprom {

void Erase(uint16_t addr, uint16_t length) {
  for (uint16_t i{}; addr + i <= E2END && i < length; ++i) {
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = addr + i;
    EECR = (EECR & ~_BV(EEPM1)) | _BV(EEPM0) | _BV(EEMPE);
    EECR |= _BV(EEPE);
  }
}

// This returns the number of bytes actually read.
uint16_t Read(uint16_t addr, void* buffer, uint16_t length) {
  auto bytes{reinterpret_cast<uint8_t*>(buffer)};
  uint16_t i{};
  for (; addr + i <= E2END && i < length; ++i) {
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = addr + i;
    EECR |= _BV(EERE);
    bytes[i] = EEDR;
  }

  return i;
}

// This only writes the EEPROM if the data in the buffer is different from what
// is already there.
void Update(uint16_t addr, const void* buffer, uint16_t length) {
  auto bytes{reinterpret_cast<const uint8_t*>(buffer)};
  for (uint16_t i{}; addr + i <= E2END && i < length; ++i) {
    uint8_t byte;
    Read(addr + i, &byte, sizeof(byte));
    if (bytes[i] == byte) continue;
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = addr + i;
    EEDR = bytes[i];
    EECR = (EECR & ~(_BV(EEPM1) | _BV(EEPM0))) | _BV(EEMPE);
    EECR |= _BV(EEPE);
  }
}

}  // namespace Eeprom