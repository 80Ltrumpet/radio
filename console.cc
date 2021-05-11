#include "console.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include <util/setbaud.h>

#include "timer.h"

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

// Stream to use for stdout/stderr.
static FILE stream{};

// Receive and transmit ring buffers for the console USART.
struct RingBuffer final {
  static constexpr uint8_t kSize{64};
  volatile uint8_t head{};
  volatile uint8_t tail{};
  char data[kSize];
};
static RingBuffer rx{};
static RingBuffer tx{};

// Console state
char Console::input[];
uint8_t Console::input_length{};
uint8_t Console::cursor{};
bool Console::need_prompt{true};
Console::Escape Console::escape{Console::Escape::None};
Console::History Console::history{};

static int PutChar(char c, FILE* stream) {
  // Precede all newlines with carriage returns.
  if (c == '\n' && PutChar('\r', stream) != 0) return _FDEV_ERR;

  // TODO: Wait until there is room in the transmit buffer.
  constexpr auto kTxTimeoutTicks{50ULL};
  unsigned long long timeout{Timer::GetTicks() + kTxTimeoutTicks};
  while (tx.tail - tx.head >= RingBuffer::kSize) {
    if (Timer::GetTicks() >= timeout) return _FDEV_ERR;
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    tx.data[tx.tail++ & (RingBuffer::kSize - 1)] = c;
    UCSR0B |= _BV(UDRIE0);
    UCSR0A |= _BV(TXC0);
  }

  return 0;
}

bool Console::Init() {
  // Set up USART0 as the console device.
  // These _VALUEs are from util/setbaud.h. BAUD is defined in the makefile.
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  // RX complete interrupt, receive, and transmit
  UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
  // 8-bit, no parity, 1 stop bit
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

  // Set up stdout/stderr for printf-related functions.
  fdev_setup_stream(&stream, PutChar, nullptr, _FDEV_SETUP_WRITE);
  stdout = &stream;
  stderr = &stream;

  // TODO: Add the console task to the scheduler.

  return true;
}

void Console::ClearInput(uint8_t length) {
  PutNChar(kKeyBackspace, cursor);
  PutNChar(' ', input_length);
  need_prompt = true;
  input_length = cursor = length;
}

bool Console::GetChar(char& c) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (rx.head >= rx.tail) {
      rx.head = rx.tail = 0;
      return false;
    }
    c = rx.data[rx.head++ & (RingBuffer::kSize - 1)];
  }

  return true;
}

void Console::PutNChar(char c, uint8_t n) {
  for (uint8_t i{0}; i < n; ++i) putchar(c);
}

bool Console::PollInput() {
  if (need_prompt) {
    need_prompt = false;
    input[input_length] = '\0';
    printf("\r> %s", input);
  }

  char c{};
  if (!GetChar(c)) {
    // No input was received.
    return false;
  }

  if (c == '\0') {
    return false;
  }

  bool newline{false};
  const uint8_t post_cursor{input_length - cursor};

  // Handle the escape state first.
  switch (escape) {
    default:
    case Escape::None:
      if (c == kKeyEscape) {
        escape = Escape::Escape;
        return false;
      }
      break;

    case Escape::Escape:
      if (c == '[') {
        escape = Escape::Bracket;
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
          escape = Escape::Esc1;
          return false;
        case '3':
          escape = Escape::Esc3;
          return false;
        default:
          // Drop unknown bracket escape sequences.
          escape = Escape::None;
          return false;
      }
      break;

    case Escape::Esc1:
      if (c != '~') {
        escape = Escape::None;
        return false;
      }
      c = kKeyHome;
      break;

    case Escape::Esc3:
      if (c != '~') {
        escape = Escape::None;
        return false;
      }
      c = kKeyDel;
      break;

    case Escape::Literal:
      escape = Escape::None;
      goto literal;
  }

  // Handle the received character.
  switch (c) {
    case kKeyNewline:
    case kKeyCarriageReturn: {
      putchar('\n');
      // Make sure the input is null-terminated.
      input[input_length] = '\0';
      need_prompt = newline = true;
      cursor = 0;
      // Add the current input to the history if it (1) is empty or
      // (2) is not the same as the most recent input.
      if (input_length == 0) {
        break;
      }
      uint8_t index{(history.latest - 1) & History::kMask};
      if (strncmp(input, history.list[index], input_length + 1) == 0) {
        history.current = history.latest;
        break;
      }
      // Copy the current input into the history.
      index = history.latest++ & History::kMask;
      strncpy(history.list[index], input, input_length + 1);
      if (history.latest - history.oldest >= History::kSize &&
          ++history.oldest >= History::kSize) {
        history.oldest &= History::kMask;
        history.latest -= History::kSize;
      }
      history.current = history.latest;
      break;
    }

    case kKeyCancel:
      input_length = 0;
      cursor = 0;
      [[fallthrough]];

    case kKeyRedraw:
    case kKeyRedraw2:
      putchar('\n');
      need_prompt = true;
      break;

    case kKeyHome:
      PutNChar(kKeyBackspace, cursor);
      cursor = 0;
      break;

    case kKeyEnd:
      input[input_length] = '\0';
      fputs(input + cursor, stdout);
      cursor = input_length;
      break;

    case kKeyUp:
      if (history.current <= history.oldest) {
        break;
      }
      if (history.current >= history.latest) {
        // Copy the current input the to latest history slot.
        input[input_length] = '\0';
        strncpy(history.list[history.latest & History::kMask], input,
                input_length + 1);
      }
      --history.current;
      goto use_history;

    case kKeyDown:
      if (history.current >= history.latest) {
        break;
      }
      ++history.current;
    use_history:
      // Replace the input with the selected history.
      strncpy(input, history.list[history.current & History::kMask],
              kInputSize);
      ClearInput(static_cast<uint8_t>(strlen(input)));
      break;

    case kKeyRight:
      if (post_cursor == 0) {
        break;
      }
      putchar(input[cursor++]);
      break;

    case kKeyLeft:
      if (cursor == 0) {
        break;
      }
      putchar(kKeyBackspace);
      --cursor;
      break;

    case kKeyClearHome:
      memmove(input, input + cursor, post_cursor);
      PutNChar(kKeyBackspace, cursor);
      input[cursor] = '\0';
      fputs(input, stdout);
      PutNChar(' ', cursor);
      PutNChar(kKeyBackspace, input_length);
      input_length = cursor;
      cursor = 0;
      break;

    case kKeyClearEnd:
      PutNChar(' ', post_cursor);
      PutNChar(kKeyBackspace, post_cursor);
      input_length = cursor;
      break;

    case kKeyDelete:
    case kKeyBackspace:
      if (cursor == 0) {
        break;
      }
      memmove(input + (cursor - 1), input + cursor, post_cursor);
      input[--input_length] = '\0';
      putchar(kKeyBackspace);
      fputs(input + --cursor, stdout);
      putchar(' ');
      PutNChar(kKeyBackspace, post_cursor + 1);
      break;

    case kKeyDel:
      if (post_cursor == 0) {
        break;
      }
      memmove(input + cursor, input + (cursor + 1), post_cursor - 1);
      input[--input_length] = '\0';
      fputs(input + cursor, stdout);
      putchar(' ');
      PutNChar(kKeyBackspace, post_cursor);
      break;

    case kKeyDeleteWord:
      if (cursor > 0) {
        uint8_t i;
        bool is_word{false};
        for (i = cursor; i > 0; --i) {
          const bool is_space = input[i - 1] == ' ';
          if (is_word && is_space) {
            break;
          }
          if (!is_space) {
            is_word = true;
          }
        }
        const uint8_t del_length{cursor - i};
        memmove(input + i, input + cursor, post_cursor);
        input_length -= del_length;
        cursor -= del_length;
        input[input_length] = '\0';
        PutNChar(kKeyBackspace, del_length);
        fputs(input + i, stdout);
        PutNChar(' ', del_length);
        PutNChar(kKeyBackspace, post_cursor + del_length);
      }
      break;

    case kKeyDeleteLine:
      ClearInput();
      break;

    case kKeyTab:
    case kKeyBeep:
      putchar(kKeyBeep);
      break;

    case kKeyLiteral:
      escape = Escape::Literal;
      return false;

    default:
      if (c < ' ' || c > '~') {
        break;
      }
    literal:
      if (input_length >= kInputSize - 1) {
        break;
      }
      if (cursor < input_length) {
        memmove(input + (cursor + 1), input + cursor, post_cursor);
      }
      input[++input_length] = '\0';
      input[cursor] = c;
      fputs(input + cursor++, stdout);
      PutNChar(kKeyBackspace, post_cursor);
      break;
  }

  escape = Escape::None;
  return newline;
}

ISR(USART0_RX_vect) {
  unsigned char parity_check{bit_is_clear(UCSR0A, UPE0)};
  char c{UDR0};
  if (parity_check) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      // Drop input that would overflow the buffer.
      if (rx.tail - rx.head < RingBuffer::kSize)
        rx.data[rx.tail++ & (RingBuffer::kSize - 1)] = c;
    }
  }
}

ISR(USART0_UDRE_vect) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (tx.head < tx.tail) {
      UDR0 = tx.data[tx.head++ & (RingBuffer::kSize - 1)];
      // Prevent overflow.
      if (tx.head >= RingBuffer::kSize) {
        tx.head -= RingBuffer::kSize;
        tx.tail -= RingBuffer::kSize;
      }
    } else {
      // There is nothing to transmit. Disable this interrupt and reset
      // the buffer.
      UCSR0B &= ~_BV(UDRIE0);
      tx.head = tx.tail = 0;
    }
  }
}