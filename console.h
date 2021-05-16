#pragma once

#include <stdint.h>
#include <stdio.h>

class Console final {
  using CommandHandler = void (*)(int, const char**);

 public:
  static void Init();

 private:
  // Size of the console input buffer
  static constexpr uint8_t kInputSize{64};

  // Maximum number of commands that can be registered
  static constexpr uint8_t kMaxCommands{1};

  // Escape states
  enum class Escape {
    None,
    Escape,
    Bracket,
    Esc1,
    Esc3,
    Literal,
  };

  // Non-printing characters
  enum Key : char {
    kKeyHome = '\x01',
    kKeyLeft = '\x02',
    kKeyCancel = '\x03',
    kKeyDel = '\x04',
    kKeyEnd = '\x05',
    kKeyRight = '\x06',
    kKeyBeep = '\x07',
    kKeyBackspace = '\x08',
    kKeyTab = '\x09',
    kKeyNewline = '\x0a',
    kKeyClearEnd = '\x0b',
    kKeyRedraw = '\x0c',
    kKeyCarriageReturn = '\x0d',
    kKeyDown = '\x0e',
    kKeyUp = '\x10',
    kKeyRedraw2 = '\x12',
    kKeyClearHome = '\x15',
    kKeyLiteral = '\x16',
    kKeyDeleteWord = '\x17',
    kKeyDeleteLine = '\x18',
    kKeyEscape = '\x1b',
    kKeyDelete = '\x7f',
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

  Console() = default;

  // Erases the input from the console and sets the input length and cursor
  // according to the length parameter.
  void clear_input(uint8_t length = 0);

  // Reads a single character from the USART RX ring buffer. If the buffer is
  // empty, this method returns false.
  bool get_char(char& c);

  // Repeats a character a given number of times.
  void put_char_n(char c, uint8_t n);

  // Handles character-by-character console input. Returns true when the
  // input buffer should be parsed.
  bool poll_input();

  // Writes a single character to the USART TX ring buffer. This method
  // supports the AVR libc stdio output API.
  static int PutChar(char c, FILE* stream);

  // Performs an iteration of the console task.
  static void Run(void* arg);

  // Stream to use for stdout/stderr
  FILE stream_{};

  // Console state
  char input_[kInputSize]{'\0'};
  uint8_t input_length_{};
  uint8_t cursor_{};
  bool need_prompt_{true};
  Escape escape_{Escape::None};
  History history_{};

  // Console singleton
  static Console console;
};