#pragma once

#include <stdint.h>

class CommandRegistry final {
  using CommandHandler = void (*)(int, const char**);

  static constexpr uint8_t kMaxCommands{2};

  class CommandRegistryImpl final {
    friend CommandRegistry;

    struct Entry final {
      const char* name{};
      CommandHandler handler{};
    };

   public:
    const Entry& get_entry(uint8_t i) const { return commands_[i]; }
    uint8_t size() const { return count_; }

   private:
    Entry commands_[kMaxCommands]{};
    uint8_t count_{};
  };

 public:
  static CommandRegistryImpl& Commands() {
    static CommandRegistryImpl registry;
    return registry;
  }

  // Returns the number of extra command "slots" in the registry. If it is
  // negative, some commands were unable to be registered (under-provisioned).
  static int8_t GetProvisioning();

  template <class Command>
  static bool RegisterCommand() {
    return CommandRegistry::RegisterCommand(Command::kCommandName,
                                            Command::CommandHandler);
  }

 private:
  static bool RegisterCommand(const char* name, CommandHandler handler);
};