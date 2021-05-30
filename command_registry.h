#pragma once

#include <stdint.h>

namespace CommandRegistry {

using CommandHandler = void (*)(int, const char**);

struct Entry final {
  const char* name{};
  CommandHandler handler{};
};

uint8_t Size();
const Entry& Get(uint8_t i);

// Returns the number of extra command "slots" in the registry. If it is
// negative, some commands were unable to be registered (under-provisioned).
int8_t GetProvisioning();

bool RegisterCommand(const char* name, CommandHandler handler);

// Utility to shorten registration boilerplate.
template <class Command>
bool RegisterCommand() {
  return RegisterCommand(Command::kCommandName, Command::CommandHandler);
}

}  // namespace CommandRegistry