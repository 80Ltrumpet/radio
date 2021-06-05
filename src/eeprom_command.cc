#ifdef ANDRUIO_ENABLE_EEPROM_COMMAND

#include <stdio.h>
#include <stdlib.h>

#include "command_registry.h"
#include "eeprom.h"

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