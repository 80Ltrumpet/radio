#include "console.h"

#include <stdio.h>
#include <string.h>

#include "command_registry.h"
#include "scheduler.h"
#include "timer.h"
#include "usb.h"

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

/*------------------------------------------------------------------------------
 * Public Static Methods
 */

void Console::Init() {
  // Add the console task to the scheduler.
  Scheduler::Task task{};
  task.name = "console";
  task.runner = Console::Run;
  Scheduler::AddTask(task);
}

/*-----------------------------------------------------------------------------
 * Private Instance Methods
 */

void Console::clear_input(uint8_t length) {
  put_char_n(kKeyBackspace, cursor_);
  put_char_n(' ', input_length_);
  need_prompt_ = true;
  input_length_ = cursor_ = length;
}

void Console::put_char_n(char c, uint8_t n) {
  for (uint8_t i{0}; i < n; ++i) putchar(c);
}

bool Console::poll_input() {
  if (need_prompt_) {
    need_prompt_ = false;
    input_[input_length_] = '\0';
    printf("\r> %s", input_);
  }

  char c{};
  if (!Usb::GetChar(c)) {
    // No input was received.
    return false;
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
      input_[cursor_] = '\0';
      fputs(input_, stdout);
      put_char_n(' ', cursor_);
      put_char_n(kKeyBackspace, input_length_);
      input_length_ = cursor_;
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

/*-----------------------------------------------------------------------------
 * Private Static Methods
 */

// Console singleton
Console Console::console{};

void Console::Run([[maybe_unused]] void* arg) {
  if (!console.poll_input()) return;

  // Generate the argument vector.
  ArgVector v{console.input_};
  if (v.argc == 0) return;

  // Dispatch the arguments to the appropriate command.
  auto& commands{CommandRegistry::Commands()};
  for (uint8_t i{}; i < commands.size(); ++i) {
    const auto& e{commands.get_entry(i)};
    if (strcmp(e.name, v.argv[0]) == 0) {
      e.handler(v.argc, v.argv);
      return;
    }
  }

  // TODO: argv[0] didn't match a known command. Maybe print some help?
  printf("Unknown command \"%s\".\n", v.argv[0]);
}