#include "timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "atomic.h"

namespace {

constexpr uint8_t kTicksPerUs{F_CPU / 1000000};  // 8
constexpr uint16_t kTicksPerOvf{256};
constexpr uint8_t kTimerPrescaler{64};
constexpr uint8_t kUsPerTimerTick{kTimerPrescaler / kTicksPerUs};
constexpr uint16_t kOvfUs{kTicksPerOvf * kUsPerTimerTick};

// Whole number of milliseconds per overflow.
constexpr uint8_t kOvfMs{kOvfUs / 1000};

// Fractional number of milliseconds per overflow. Shifing right by three allows
// it to fit in a byte without loss of precision (because F_CPU is a multiple of
// eight).
constexpr uint8_t kOvfMsFrac{(kOvfUs % 1000) >> 3};
constexpr uint8_t kMsFracMax{1000 >> 3};  // 125

volatile uint64_t g_ovf_count{};
volatile uint64_t g_millis{};
uint8_t g_frac{};

}  // namespace

namespace Timer {

void Init() {
  TCNT0 = 0;
  TCCR0A |= _BV(WGM01) | _BV(WGM00);  // Fast PWM mode
  TCCR0B |= _BV(CS01) | _BV(CS00);    // 64x prescaler
  TIMSK0 |= _BV(TOIE0);               // Overflow interrupt
}

uint64_t Millis() {
  uint64_t millis;
  {
    AtomicLock lock{};
    millis = g_millis;
  }
  return millis;
}

uint64_t Micros() {
  uint64_t ovf_count;
  uint8_t ticks;
  {
    AtomicLock lock{};
    ovf_count = g_ovf_count;
    ticks = TCNT0;

    // Check if an overflow occurred before reading TCNT0.
    if ((TIFR0 & _BV(TOV0)) && ticks <= TCNT0) {
      ++ovf_count;
    }
  }

  return ((ovf_count << 8) | ticks) * kUsPerTimerTick;
}

void DelayMs(uint64_t ms) {
  // Use microseconds to avoid resolution issues.
  for (auto start{Micros()}; Micros() - start < ms * 1000;)
    ;
}

void DelayUs(uint32_t us) {
  // Function call overhead is 14 cycles, which is almost 2 microseconds at
  // 8 MHz. If the argument is less than 3, just return.
  if (us < 3) return;  // 3 cycles (4 when true)

  // The ASM loop takes 4 cycles (0.5 microseconds) per iteration, so it needs
  // to run twice per requested microsecond. However, accounting for the cycles
  // burned so far (including these adjustments), we need to subtract 5 (since
  // 5 * 4 = 16, which is close enough to 21).
  us = (us << 1) - 5;  // 4 cycles

  // Busy wait
  __asm__ __volatile__(
      "1: sbiw %0,1"  // 2 cycles
      "\n\t"
      "brne 1b"  // 2 cycles
      : "=w"(us)
      : "0"(us));

  // Returning takes 4 cycles.
}

}  // namespace Timer

ISR(TIMER0_OVF_vect) {
  // Copy volatile variable to registers for speed.
  uint64_t millis{g_millis};

  millis += kOvfMs;
  g_frac += kOvfMsFrac;
  if (g_frac >= kMsFracMax) {
    g_frac -= kMsFracMax;
    ++millis;
  }

  g_millis = millis;
  ++g_ovf_count;
}