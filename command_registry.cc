#include "command_registry.h"

int8_t CommandRegistry::GetProvisioning() {
  return static_cast<int8_t>(kMaxCommands) -
         static_cast<int8_t>(Commands().count_);
}

bool CommandRegistry::RegisterCommand(const char* name,
                                      CommandHandler handler) {
  auto& registry{Commands()};
  auto success{registry.count_ < kMaxCommands};
  if (success) {
    auto& entry{registry.commands_[registry.count_]};
    entry.name = name;
    entry.handler = handler;
  }

  // Always increment the count to aid in verification of static provisioning.
  ++registry.count_;
  return success;
}