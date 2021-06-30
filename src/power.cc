#include "power.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "atomic.h"
#include "led.h"

namespace {

constexpr uint8_t kTicksPerUs{F_CPU / 1000000};
constexpr uint16_t kTimerPrescaler{1024};
constexpr uint32_t kUsPerTimerTick{kTimerPrescaler / kTicksPerUs};
constexpr uint16_t kTimerMaxMs{UINT16_MAX * kUsPerTimerTick / 1000};

using TimerMutator = void (*)(uint16_t);

TimerMutator timer_mutator_{};

}  // namespace

namespace Power {

void Init() {
  // Configure the sleep timer for long-term, low-resolution, operation.
  // Note: This timer is disabled below. It is enabled when needed in Sleep().
  OCR1A = UINT16_MAX;
  TCCR1A = 0;
  TCCR1B = _BV(CS12) | _BV(CS10);  // 1024x prescaler
  TIMSK1 = _BV(OCIE1A);
  TIFR1 = _BV(OCF1A);

  // Disable the analog comparator.
  ACSR = _BV(ACD);

  // Disable other unused functions.
#define DISABLE 1
#define ENABLE 0
  PRR0 = 0
#if defined(__AVR_HAVE_PRR0_PRTWI) && DISABLE
         | _BV(PRTWI)
#endif
#if defined(__AVR_HAVE_PRR0_PRTIM2) && DISABLE
         | _BV(PRTIM2)
#endif
#if defined(__AVR_HAVE_PRR0_PRTIM0) && ENABLE
         | _BV(PRTIM0)
#endif
#if defined(__AVR_HAVE_PRR0_PRTIM1) && DISABLE
         | _BV(PRTIM1)
#endif
#if defined(__AVR_HAVE_PRR0_PRSPI) && ENABLE
         | _BV(PRSPI)
#endif
#if defined(__AVR_HAVE_PRR0_PRUSART0) && !defined(BAUD)
         | _BV(PRUSART0)
#endif
#if defined(__AVR_HAVE_PRR0_PRADC) && DISABLE
         | _BV(PRADC)
#endif
      ;

  PRR1 = 0
#if defined(__AVR_HAVE_PRR1_PRUSB) && ENABLE
         | _BV(PRUSB)
#endif
#if defined(__AVR_HAVE_PRR1_PRTIM5) && DISABLE
         | _BV(PRTIM5)
#endif
#if defined(__AVR_HAVE_PRR1_PRTIM4) && DISABLE
         | _BV(PRTIM4)
#endif
#if defined(__AVR_HAVE_PRR1_PRTIM3) && DISABLE
         | _BV(PRTIM3)
#endif
#if defined(__AVR_HAVE_PRR1_PRUSART3) && DISABLE
         | _BV(PRUSART3)
#endif
#if defined(__AVR_HAVE_PRR1_PRUSART2) && DISABLE
         | _BV(PRUSART2)
#endif
#if defined(__AVR_HAVE_PRR1_PRUSART1) && DISABLE
         | _BV(PRUSART1)
#endif
      ;
#undef ENABLE
#undef DISABLE
}

void SetTimerMutator(TimerMutator mutator) { timer_mutator_ = mutator; }

void Sleep(uint16_t sleep_ms) {
#ifdef UDIEN
  // USB Start-Of-Frame interrupts will immediately wake up the MCU.
  UDIEN &= ~_BV(SOFE);
#endif
  // Timer/counter 0 interrupts will immediately wake up the MCU.
  PRR0 |= _BV(PRTIM0);

  if (sleep_ms > 0) {
    sleep_ms = sleep_ms < kTimerMaxMs ? sleep_ms : kTimerMaxMs;
  } else {
    sleep_ms = kTimerMaxMs;
  }

  {
    AtomicLock lock{};
    // Enable timer/counter 1 (sleep timer).
    PRR0 &= ~_BV(PRTIM1);
    // Due to truncation, this is guaranteed not to exceed UINT16_MAX.
    OCR1A = sleep_ms * 1000UL / kUsPerTimerTick;
    TCNT1 = 0;
    TIFR1 = _BV(OCF1A);
  }

  // Turn off the LED while asleep.
  Led::Off();

  // Enable the Idle sleep mode and execute the sleep instruction.
  SMCR = _BV(SE);
  __asm__ __volatile__(
      "sleep"
      "\n\t" ::);
  SMCR = 0;

  Led::On();

  // Disable the sleep timer and mutate the event timer.
  if (timer_mutator_) {
    {
      AtomicLock lock{};
      sleep_ms = TCNT1 * kUsPerTimerTick / 1000;
    }
    PRR0 |= _BV(PRTIM1);
    timer_mutator_(sleep_ms);
  }

  // Reenable the quashed interrupts.
  PRR0 &= ~_BV(PRTIM0);
#ifdef UDIEN
  UDIEN |= _BV(SOFE);
#endif
}

}  // namespace Power

ISR(TIMER1_COMPA_vect) {}