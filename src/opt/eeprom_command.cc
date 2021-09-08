#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"
#include "eeprom.h"

struct EepromCommand final
{
  static void CommandHandler(int argc, const char *argv[]);
  static const char *const kCommandName;

private:
  static const bool registered;
};

// TODO: This is super hacky and ugly and barely usable.
void EepromCommand::CommandHandler(int argc, const char *argv[])
{
  if (argc < 4) {
usage:
    puts("Usage: eeprom read  ADDRESS LENGTH\n"
         "              write ADDRESS BYTE");
    return;
  }

  // Get the common address argument first.
  auto addr{atoi(argv[2])};

  if (strcmp(argv[1], "read") == 0) {
    uint16_t len{atoi(argv[3])};

    // Read up to 64B at a time.
    constexpr uint16_t kBufLen{64};
    uint8_t buffer[kBufLen];
    auto rlen{kBufLen};
    for (uint16_t offset{}; offset < len; offset += rlen)
    {
      const uint16_t remaining{len - offset};
      rlen = remaining < kBufLen ? remaining : kBufLen;
      auto read{Eeprom::Read(Eeprom::Datum{addr + offset, rlen}, buffer)};
      // Print up to eight bytes per line.
      for (uint16_t i{}; i < read; ++i)
      {
        printf("%02" PRIx8 " ", buffer[i]);
        if ((i & 7) == 7)
          puts("");
      }
    }
    puts("");
  } else if (strcmp(argv[1], "write") == 0) {
    uint8_t byte{atoi(argv[3])};
    Eeprom::Update(Eeprom::Datum{addr, 1}, &byte);
  } else {
    goto usage;
  }
}

const char *const EepromCommand::kCommandName{"eeprom"};
const bool EepromCommand::registered{
    CommandRegistry::RegisterCommand<EepromCommand>()};
