#include "usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>

#include "atomic.h"
#include "console.h"
#include "ring_buffer.h"
#include "timer.h"

namespace {

// USART transmit/receive ring buffers
RingBuffer<char, 64> tx_{};
RingBuffer<char, 64> rx_{};

// Standard output/error stream
FILE stream_{};

// Input hook for Console
bool get_char(char& c) {
  AtomicLock lock{};
  if (rx_.empty()) {
    return false;
  }

  c = rx_.pop_front();
  return true;
}

// Output hook for stdio
int put_char(char c, FILE* stream) {
  if (c == '\n' && put_char('\r', stream) != 0) return _FDEV_ERR;

  // Wait until there is room in the transmit buffer.
  constexpr uint64_t kTxTimeoutMs{5};
  const auto timeout{Timer::Millis() + kTxTimeoutMs};
  while (tx_.full()) {
    if (Timer::Millis() >= timeout) return _FDEV_ERR;
  }

  AtomicLock lock{};
  tx_.push_back(c);
  UCSR0B |= _BV(UDRIE0);
  UCSR0A |= _BV(TXC0);

  return 0;
}

}  // namespace

namespace Usart {

void Init() {
  // Set up USART0 for standard I/O.
  // These _VALUEs are from util/setbaud.h.
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  // RX complete interrupt, receive, and transmit
  UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
  // 8-bit, no parity, 1 stop bit
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

  // Set up stdout/stderr.
  fdev_setup_stream(&stream_, put_char, nullptr, _FDEV_SETUP_WRITE);
  stdout = &stream_;
  stderr = &stream_;

  // Hook input into the console.
  Console::SetGetChar(get_char);
}

}  // namespace Usart

ISR(USART0_RX_vect) {
  auto parity_check{bit_is_clear(UCSR0A, UPE0)};
  char c{UDR0};
  if (parity_check) {
    // Drop input that would overflow the buffer.
    if (!rx_.full()) {
      rx_.push_back(c);
    }
  }

  Console::Notify();
}

ISR(USART0_UDRE_vect) {
  if (tx_.empty()) {
    // There is nothing to transmit. Disable this interrupt and reset the
    // buffer.
    UCSR0B &= ~_BV(UDRIE0);
    tx_.clear();
  } else {
    UDR0 = tx_.pop_front();
  }
}