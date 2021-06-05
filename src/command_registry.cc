#include "command_registry.h"

#include <stdio.h>

#include "scheduler.h"

namespace {

constexpr uint8_t kMaxCommands{ANDRUIO_MAX_COMMANDS};

struct CommandRegistryImpl final {
  CommandRegistry::Entry entries[kMaxCommands]{};
  uint8_t count{};
};

// Avoids the static initialization order fiasco. Note that this is only
// necessary because CommandRegistry::RegisterCommand is used for static
// initialization.
CommandRegistryImpl& impl() {
  static CommandRegistryImpl impl_;
  return impl_;
}

}  // namespace

namespace CommandRegistry {

uint8_t Size() {
  return impl().count < kMaxCommands ? impl().count : kMaxCommands;
}

const Entry& Get(uint8_t i) { return impl().entries[i]; }

int8_t GetProvisioning() {
  return static_cast<int8_t>(kMaxCommands) - static_cast<int8_t>(impl().count);
}

bool RegisterCommand(const char* name, CommandHandler handler) {
  auto& registry{impl()};
  auto success{registry.count < kMaxCommands};
  if (success) {
    auto& entry{registry.entries[registry.count]};
    entry.name = name;
    entry.handler = handler;
  }

  // Always increment the count to aid in verification of static provisioning.
  ++registry.count;
  return success;
}

}  // namespace CommandRegistry

/*------------------------------------------------------------------------------
 * Built-in "verify" command
 */
struct VerifyCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void VerifyCommand::CommandHandler(int argc, const char* argv[]) {
  printf("Commands: %3" PRId8 "\nTasks:    %3" PRId8 "\n",
         CommandRegistry::GetProvisioning(), Scheduler::GetProvisioning());
}

const char* const VerifyCommand::kCommandName{"verify"};
const bool VerifyCommand::registered{
    CommandRegistry::RegisterCommand<VerifyCommand>()};