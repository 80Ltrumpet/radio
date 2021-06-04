#include "eeprom.h"

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "command_registry.h"

#if 0

/*------------------------------------------------------------------------------
 * EEPROM command
 */
struct EepromCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void EepromCommand::CommandHandler(int argc, const char* argv[]) {
  if (argc < 2) {
    puts("Usage: eeprom ADDRESS [LENGTH]");
    return;
  }

  auto addr{atoi(argv[1])};
  uint16_t len{1};
  if (argc > 2) {
    len = atoi(argv[2]);
  }

  // Read up to 64B at a time.
  constexpr uint16_t kBufLen{64};
  uint8_t buffer[kBufLen];
  auto rlen{kBufLen};
  for (uint16_t offset{}; offset < len; offset += rlen) {
    const uint16_t remaining{len - offset};
    rlen = remaining < kBufLen ? remaining : kBufLen;
    auto read{Eeprom::Read(addr + offset, buffer, rlen)};
    // Print up to eight bytes per line.
    for (uint16_t i{}; i < read; ++i) {
      printf("%02" PRIx8 " ", buffer[i]);
      if ((i & 7) == 7) puts("");
    }
  }
}

const char* const EepromCommand::kCommandName{"eeprom"};
const bool EepromCommand::registered{
    CommandRegistry::RegisterCommand<EepromCommand>()};

#endif

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
  for (uint16_t i{}; addr + i <= E2END && i < length; ++i) {
    AtomicLock lock{};
    while (EECR & _BV(EEPE))
      ;
    EEAR = addr + i;
    EECR |= _BV(EERE);
    bytes[i] = EEDR;
  }
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