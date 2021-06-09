#include "console.h"

#include <stdio.h>
#include <string.h>

#include "atomic.h"
#include "command_registry.h"
#include "scheduler.h"
#include "timer.h"

namespace {

// Size of the console input buffer
constexpr uint8_t kInputSize{64};

}  // namespace

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

/*------------------------------------------------------------------------------
 * Argument vector utility
 */
class ArgVector final {
  // Maximum number of arguments
  static constexpr int kMaxArgc{16};

 public:
  // The input must be null-terminated.
  ArgVector(char* input);

  int argc{};
  const char* argv[kMaxArgc]{};
};

ArgVector::ArgVector(char* input) {
  const char* arg_start{};
  for (; *input != '\0'; ++input) {
    const auto is_space{*input == ' '};
    if (arg_start) {
      if (is_space) {
        *input = '\0';
        argv[argc++] = arg_start;
        arg_start = nullptr;
      }
    } else if (!is_space) {
      arg_start = input;
    }
  }
  if (arg_start && argc < kMaxArgc) {
    argv[argc++] = arg_start;
  }
}

/*-----------------------------------------------------------------------------
 * Private stuff
 */
namespace {

using GetCharFunc = bool (*)(char&);

// Hook for "console efficient" getc()
GetCharFunc get_char_func_{};

// Console state
char input_[kInputSize]{'\0'};
uint8_t input_length_{};
uint8_t cursor_{};
bool need_prompt_{true};
Escape escape_{Escape::None};
History history_{};

TaskHandle task_{};

// Gets a single character of input.
bool get_char(char& c) { return get_char_func_ ? get_char_func_(c) : false; }

// Repeats a character a given number of times.
void put_char_n(char c, uint8_t n) {
  for (uint8_t i{0}; i < n; ++i) putchar(c);
}

// Erases the input from the console and sets the input length and cursor
// according to the length parameter.
void clear_input(uint8_t length = 0) {
  put_char_n(kKeyBackspace, cursor_);
  put_char_n(' ', input_length_);
  need_prompt_ = true;
  input_length_ = cursor_ = length;
}

// Handles character-by-character console input. Returns true when the
// input buffer should be parsed.
bool poll_input() {
  if (need_prompt_) {
    need_prompt_ = false;
    input_[input_length_] = '\0';
    printf("\r> %s", input_);
  }

  char c{};
  {
    // These operations are performed atomically to prevent a race condition
    // between checking for a character and a new character being received.
    // This ensures the console task will not be paused when input is available.
    AtomicLock lock{};
    if (!get_char(c)) {
      // No input was received.
      Scheduler::PauseTask(task_);
      return false;
    }
  }

  if (c == '\0') {
    return false;
  }

  bool newline{false};
  const uint8_t post_cursor{input_length_ - cursor_};

  // Handle the escape state first.
  switch (escape_) {
    default:
    case Escape::None:
      if (c == kKeyEscape) {
        escape_ = Escape::Escape;
        return false;
      }
      break;

    case Escape::Escape:
      if (c == '[') {
        escape_ = Escape::Bracket;
        return false;
      }
      break;

    case Escape::Bracket:
      switch (c) {
        case 'A':
          c = kKeyUp;
          break;
        case 'B':
          c = kKeyDown;
          break;
        case 'C':
          c = kKeyRight;
          break;
        case 'D':
          c = kKeyLeft;
          break;
        case 'F':
          c = kKeyEnd;
          break;
        case 'M':
          c = kKeyCarriageReturn;
          break;
        case '1':
          escape_ = Escape::Esc1;
          return false;
        case '3':
          escape_ = Escape::Esc3;
          return false;
        default:
          // Drop unknown bracket escape sequences.
          escape_ = Escape::None;
          return false;
      }
      break;

    case Escape::Esc1:
      if (c != '~') {
        escape_ = Escape::None;
        return false;
      }
      c = kKeyHome;
      break;

    case Escape::Esc3:
      if (c != '~') {
        escape_ = Escape::None;
        return false;
      }
      c = kKeyDel;
      break;

    case Escape::Literal:
      escape_ = Escape::None;
      goto literal;
  }

  // Handle the received character.
  switch (c) {
    case kKeyNewline:
    case kKeyCarriageReturn: {
      putchar('\n');
      // Make sure the input is null-terminated.
      input_[input_length_] = '\0';
      need_prompt_ = newline = true;
      cursor_ = 0;
      // Add the current input to the history if it (1) is empty or
      // (2) is not the same as the most recent input.
      if (input_length_ == 0) {
        break;
      }
      uint8_t index{(history_.latest - 1) & History::kMask};
      if (strncmp(input_, history_.list[index], input_length_ + 1) == 0) {
        goto fast_forward_history;
      }
      // Copy the current input into the history.
      index = history_.latest++ & History::kMask;
      strncpy(history_.list[index], input_, input_length_ + 1);
      if (history_.latest - history_.oldest >= History::kSize &&
          ++history_.oldest >= History::kSize) {
        history_.oldest &= History::kMask;
        history_.latest -= History::kSize;
      }
    fast_forward_history:
      history_.current = history_.latest;
      // Reset input length for the next command. The input buffer can be
      // safely processed as a null-terminated string.
      input_length_ = 0;
      break;
    }

    case kKeyCancel:
      input_length_ = 0;
      cursor_ = 0;
      [[fallthrough]];

    case kKeyRedraw:
    case kKeyRedraw2:
      putchar('\n');
      need_prompt_ = true;
      break;

    case kKeyHome:
      put_char_n(kKeyBackspace, cursor_);
      cursor_ = 0;
      break;

    case kKeyEnd:
      input_[input_length_] = '\0';
      fputs(input_ + cursor_, stdout);
      cursor_ = input_length_;
      break;

    case kKeyUp:
      if (history_.current <= history_.oldest) {
        break;
      }
      if (history_.current >= history_.latest) {
        // Copy the current input the to latest history slot.
        input_[input_length_] = '\0';
        strncpy(history_.list[history_.latest & History::kMask], input_,
                input_length_ + 1);
      }
      --history_.current;
      goto use_history;

    case kKeyDown:
      if (history_.current >= history_.latest) {
        break;
      }
      ++history_.current;
    use_history:
      // Replace the input with the selected history.
      strncpy(input_, history_.list[history_.current & History::kMask],
              kInputSize);
      clear_input(static_cast<uint8_t>(strlen(input_)));
      break;

    case kKeyRight:
      if (post_cursor == 0) {
        break;
      }
      putchar(input_[cursor_++]);
      break;

    case kKeyLeft:
      if (cursor_ == 0) {
        break;
      }
      putchar(kKeyBackspace);
      --cursor_;
      break;

    case kKeyClearHome:
      memmove(input_, input_ + cursor_, post_cursor);
      put_char_n(kKeyBackspace, cursor_);
      input_[post_cursor] = '\0';
      fputs(input_, stdout);
      put_char_n(' ', cursor_);
      put_char_n(kKeyBackspace, input_length_);
      input_length_ = post_cursor;
      cursor_ = 0;
      break;

    case kKeyClearEnd:
      put_char_n(' ', post_cursor);
      put_char_n(kKeyBackspace, post_cursor);
      input_length_ = cursor_;
      break;

    case kKeyDelete:
    case kKeyBackspace:
      if (cursor_ == 0) {
        break;
      }
      memmove(input_ + (cursor_ - 1), input_ + cursor_, post_cursor);
      input_[--input_length_] = '\0';
      putchar(kKeyBackspace);
      fputs(input_ + --cursor_, stdout);
      putchar(' ');
      put_char_n(kKeyBackspace, post_cursor + 1);
      break;

    case kKeyDel:
      if (post_cursor == 0) {
        break;
      }
      memmove(input_ + cursor_, input_ + (cursor_ + 1), post_cursor - 1);
      input_[--input_length_] = '\0';
      fputs(input_ + cursor_, stdout);
      putchar(' ');
      put_char_n(kKeyBackspace, post_cursor);
      break;

    case kKeyDeleteWord:
      if (cursor_ > 0) {
        uint8_t i;
        bool is_word{false};
        for (i = cursor_; i > 0; --i) {
          const bool is_space = input_[i - 1] == ' ';
          if (is_word && is_space) {
            break;
          }
          if (!is_space) {
            is_word = true;
          }
        }
        const uint8_t del_length{cursor_ - i};
        memmove(input_ + i, input_ + cursor_, post_cursor);
        input_length_ -= del_length;
        cursor_ -= del_length;
        input_[input_length_] = '\0';
        put_char_n(kKeyBackspace, del_length);
        fputs(input_ + i, stdout);
        put_char_n(' ', del_length);
        put_char_n(kKeyBackspace, post_cursor + del_length);
      }
      break;

    case kKeyDeleteLine:
      clear_input();
      break;

    case kKeyTab:
    case kKeyBeep:
      putchar(kKeyBeep);
      break;

    case kKeyLiteral:
      escape_ = Escape::Literal;
      return false;

    default:
      if (c < ' ' || c > '~') {
        break;
      }
    literal:
      if (input_length_ >= kInputSize - 1) {
        break;
      }
      if (cursor_ < input_length_) {
        memmove(input_ + (cursor_ + 1), input_ + cursor_, post_cursor);
      }
      input_[++input_length_] = '\0';
      input_[cursor_] = c;
      fputs(input_ + cursor_++, stdout);
      put_char_n(kKeyBackspace, post_cursor);
      break;
  }

  escape_ = Escape::None;
  return newline;
}

// Performs an iteration of the console task.
void run() {
  if (!poll_input()) return;

  // Generate the argument vector.
  ArgVector v{input_};
  if (v.argc == 0) return;

  // Dispatch the arguments to the appropriate command.
  const auto size{CommandRegistry::Size()};
  for (uint8_t i{}; i < size; ++i) {
    const auto& e{CommandRegistry::Get(i)};
    if (strcmp(e.name, v.argv[0]) == 0) {
      e.handler(v.argc, v.argv);
      return;
    }
  }

  // TODO: argv[0] didn't match a known command. Maybe print some help?
  printf("Unknown command \"%s\".\n", v.argv[0]);
}

}  // namespace

/*------------------------------------------------------------------------------
 * Public Console functions
 */

namespace Console {

void Init() {
  // Add the console task to the scheduler.
  task_ = Scheduler::AddTask({"console", run});
}

void SetGetChar(GetCharFunc func) { get_char_func_ = func; }

void Notify() { 
  Scheduler::RestartTask(task_);
}

}  // namespace Console