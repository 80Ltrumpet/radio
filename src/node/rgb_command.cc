#include <stdio.h>
#include <stdlib.h>

#include "command_registry.h"
#include "rgb.h"

struct RgbCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void RgbCommand::CommandHandler(int argc, const char* argv[]) {
  static const char* const kMsgInvalid{"Invalid argument(s)"};
  if (argc != 7) {
    puts("Usage: rgb R G B PATTERN PERIOD_MS TRANS_MS");
    return;
  }

  char* endptr{};
  auto red{strtol(argv[1], &endptr, 0)};
  if (*endptr != '\0' || red < 0 || red > UINT8_MAX) {
    puts(kMsgInvalid);
    return;
  }

  endptr = nullptr;
  auto green{strtol(argv[2], &endptr, 0)};
  if (*endptr != '\0' || green < 0 || green > UINT8_MAX) {
    puts(kMsgInvalid);
    return;
  }

  endptr = nullptr;
  auto blue{strtol(argv[3], &endptr, 0)};
  if (*endptr != '\0' || blue < 0 || blue > UINT8_MAX) {
    puts(kMsgInvalid);
    return;
  }

  endptr = nullptr;
  auto pattern{strtol(argv[4], &endptr, 10)};
  if (*endptr != '\0' || pattern < 0 || pattern > 4) {
    puts(kMsgInvalid);
    return;
  }

  endptr = nullptr;
  auto period_ms{strtol(argv[5], &endptr, 10)};
  if (*endptr != '\0' || period_ms < 0) {
    puts(kMsgInvalid);
    return;
  }

  endptr = nullptr;
  auto trans_ms{strtol(argv[6], &endptr, 10)};
  if (*endptr != '\0' || trans_ms < 0) {
    puts(kMsgInvalid);
    return;
  }

  Rgb::Pattern patt_enum;
  if (pattern == 1) {
    patt_enum = Rgb::Pattern::Blink;
  } else if (pattern == 2) {
    patt_enum = Rgb::Pattern::Throb;
  } else if (pattern == 3) {
    patt_enum = Rgb::Pattern::SineOff;
  } else if (pattern == 4) {
    patt_enum = Rgb::Pattern::SineWhite;
  } else {
    patt_enum = Rgb::Pattern::None;
  }

  Rgb::Mutator{}
      .set_color({red, green, blue})
      .set_pattern(patt_enum)
      .set_period(period_ms)
      .set_transition_period(trans_ms);
}

const char* const RgbCommand::kCommandName{"rgb"};
const bool RgbCommand::registered{
    CommandRegistry::RegisterCommand<RgbCommand>()};
