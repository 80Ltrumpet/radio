#pragma once

#include <stdint.h>
#include <stdlib.h>

class Console final {
 public:
  // This class is purely static, so don't allow instantiation.
  Console() = delete;

  static bool Init();

  // This template defines an auto-registering console command.
  template <class C>
  class Command {
   public:
    friend C;

    static bool RegisterCommand() {
      auto node{new Console::CommandList(C::kCommandName, C::Execute)};
      if (!node) return false;
      node->next = Console::Commands();
      Console::Commands() = node;
      return true;
    }

   private:
    static bool registered;
  };

 private:
  static constexpr uint8_t kInputSize{64};

  // Escape states
  enum class Escape {
    None,
    Escape,
    Bracket,
    Esc1,
    Esc3,
    Literal,
  };

  // Buffer for console history
  struct History final {
    static constexpr uint8_t kSize{8};
    static constexpr uint8_t kMask{kSize - 1};
    char list[kSize][kInputSize];
    uint8_t oldest{};
    uint8_t latest{};
    uint8_t current{};
  };

  // Linked list of registered commands
  struct CommandList final {
    using Handler = void (*)(int, const char**);
    CommandList() = delete;
    CommandList(const char* name_, Handler exec_) : name{name_}, exec{exec_} {}
    const char* name{nullptr};
    Handler exec{nullptr};
    CommandList* next{nullptr};
  };

  // Erases the input from the console and sets the input length and cursor
  // according to the length parameter.
  static void ClearInput(uint8_t length = 0);

  // Gets a single character from the USART RX ring buffer. If the buffer is
  // empty, this method returns false.
  static bool GetChar(char& c);

  // Repeats a character a given number of times.
  static void PutNChar(char c, uint8_t n);

  // Handles character-by-character console input. Returns true when the
  // input buffer should be parsed.
  static bool PollInput();

  // Gets the list of registered commands.
  static CommandList*& Commands() {
    static CommandList* command_list{nullptr};
    return command_list;
  }

  // Console state
  static char input[kInputSize];
  static uint8_t input_length;
  static uint8_t cursor;
  static bool need_prompt;
  static Escape escape;
  static History history;
};

template <class C>
bool Console::Command<C>::registered =
    Console::Command<C>::RegisterCommand();