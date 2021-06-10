#include "timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "atomic.h"
#include "power.h"

namespace {

constexpr uint8_t kTicksPerUs{F_CPU / 1000000};
constexpr uint16_t kTicksPerOvf{256};
constexpr uint8_t kTimerPrescaler{64};
constexpr uint8_t kUsPerTimerTick{kTimerPrescaler / kTicksPerUs};
constexpr uint16_t kOvfUs{kTicksPerOvf * kUsPerTimerTick};

// Whole number of milliseconds per overflow.
constexpr uint8_t kOvfMs{kOvfUs / 1000};

// Fractional number of milliseconds per overflow. Shifting right by three
// allows it to fit in a byte without loss of precision (because F_CPU is a
// multiple of eight).
constexpr uint8_t kOvfMsFrac{(kOvfUs % 1000) >> 3};
constexpr uint8_t kMsFracMax{1000 >> 3};  // 125

volatile uint64_t ovf_count_{};
volatile uint64_t millis_{};
uint8_t frac_{};

}  // namespace

namespace Timer {

void Init() {
  TCNT0 = 0;
  TCCR0A |= _BV(WGM01) | _BV(WGM00);  // Fast PWM mode
  TCCR0B |= _BV(CS01) | _BV(CS00);    // 64x prescaler
  TIMSK0 |= _BV(TOIE0);               // Overflow interrupt

  // This is called when timer/counter 0 and its related interrupts are
  // disabled, so this will not race.
  Power::SetTimerMutator([](uint16_t ms) { millis_ += ms; });
}

uint64_t Millis() {
  uint64_t millis;
  {
    AtomicLock lock{};
    millis = millis_;
  }
  return millis;
}

uint64_t Micros() {
  uint64_t ovf_count;
  uint8_t ticks;
  {
    AtomicLock lock{};
    ovf_count = ovf_count_;
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
#if F_CPU == 16000000UL
  // Funciton call overhead is 14 cycles, which is almost 1 microsecond at
  // 16 MHz. If the argument is less than 2, just return.
  if (us < 2) return;  // 3 cycles (4 when true)

  // The ASM loop takes 4 cycles (0.25 microseconds) per iteration, so it needs
  // to run four times per requested microsecond. However, accounting for the
  // cycles burned so far (including these adjustments) and the 4-cycle return,
  // we need to subtract 6 (since 6 * 4 = 24, which is close enough to 27).
  us = (us << 2) - 6;  // 6 cycles
#elif F_CPU == 8000000L
  // Function call overhead is 14 cycles, which is almost 2 microseconds at
  // 8 MHz. If the argument is less than 3, just return.
  if (us < 3) return;  // 3 cycles (4 when true)
  if (us < 4) return;  // 3 cycles (4 when true)

  // The ASM loop takes 4 cycles (0.5 microseconds) per iteration, so it needs
  // to run twice per requested microsecond. However, accounting for the cycles
  // burned so far (including these adjustments) and the 4-cycle return, we need
  // to subtract 7 (since 7 * 4 = 28).
  us = (us << 1) - 7;  // 4 cycles
#else
#error "Timer::DelayUs does not support F_CPU."
#endif

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
  uint64_t millis{millis_};

  millis += kOvfMs;
  frac_ += kOvfMsFrac;
  if (frac_ >= kMsFracMax) {
    frac_ -= kMsFracMax;
    ++millis;
  }

  millis_ = millis;
  ++ovf_count_;
}