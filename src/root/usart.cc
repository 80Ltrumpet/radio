#include "usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>

#include "atomic.h"
#include "console.h"
#include "timer.h"

// Ring buffer for USART TX/RX.
struct RingBuffer final {
  static constexpr uint8_t kSize{64};
  volatile uint8_t head{};
  volatile uint8_t tail{};
  char data[kSize];
};

namespace {

// USART transmit/receive ring buffers
RingBuffer tx_{};
RingBuffer rx_{};

// Standard output/error stream
FILE stream_{};

// Input hook for Console
bool get_char(char& c) {
  AtomicLock lock{};
  if (rx_.head >= rx_.tail) {
    rx_.head = rx_.tail = 0;
    return false;
  }

  c = rx_.data[rx_.head++ & (RingBuffer::kSize - 1)];
  return true;
}

// Output hook for stdio
int put_char(char c, FILE* stream) {
  if (c == '\n' && put_char('\r', stream) != 0) return _FDEV_ERR;

  // Wait until there is room in the transmit buffer.
  constexpr uint64_t kTxTimeoutMs{5};
  const auto timeout{Timer::Millis() + kTxTimeoutMs};
  while (tx_.tail - tx_.head >= RingBuffer::kSize) {
    if (Timer::Millis() >= timeout) return _FDEV_ERR;
  }

  AtomicLock lock{};
  tx_.data[tx_.tail++ & (RingBuffer::kSize - 1)] = c;
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
    if (rx_.tail - rx_.head < RingBuffer::kSize) {
      rx_.data[rx_.tail++ & (RingBuffer::kSize - 1)] = c;
    }
  }

  Console::Notify();
}

ISR(USART0_UDRE_vect) {
  if (tx_.head < tx_.tail) {
    UDR0 = tx_.data[tx_.head++ & (RingBuffer::kSize - 1)];
    // Prevent overflow.
    if (tx_.head >= RingBuffer::kSize) {
      tx_.head -= RingBuffer::kSize;
      tx_.tail -= RingBuffer::kSize;
    }
  } else {
    // There is nothing to transmit. Disable this interrupt and reset the
    // buffer.
    UCSR0B &= ~_BV(UDRIE0);
    tx_.head = tx_.tail = 0;
  }
}