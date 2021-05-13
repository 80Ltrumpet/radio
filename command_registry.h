#pragma once

#include <stdint.h>

class CommandRegistry {
  using CommandHandler = void (*)(int, const char**);

 public:
  template <class C>
  class Command {
    friend C;

   public:
    static bool Register() {
      return Commands().add_entry(C::kCommandName, C::CommandHandler);
    }

   private:
    Command() { (void)registered; }

    static bool registered;
  };

 protected:
  static constexpr uint8_t kMaxCommands{};

  class Registry final {
   public:
    bool add_entry(const char* name, CommandHandler handler);

   private:
    struct Entry final {
      const char* name{};
      CommandHandler handler{};
    };

    Entry commands_[kMaxCommands]{};
    uint8_t count_{};
  };

  CommandRegistry() = default;

  static Registry& Commands() {
    static Registry registry;
    return registry;
  }
};

template <class C>
bool CommandRegistry::Command<C>::registered =
    CommandRegistry::Command<C>::Register();