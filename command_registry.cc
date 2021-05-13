#include "command_registry.h"

bool CommandRegistry::Registry::add_entry(const char* name,
                                          CommandHandler handler) {
  auto success{count_ < kMaxCommands};
  if (success) {
    auto& entry{commands_[count_]};
    entry.name = name;
    entry.handler = handler;
  }

  // TODO: Perform this static provisioning verification somewhere...
  // Always increment the count to aid in verification of static provisioning.
  ++count_;
  return success;
}