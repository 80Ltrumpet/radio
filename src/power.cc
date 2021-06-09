#include "power.h"

#include <avr/io.h>

#if defined(ARDUINO_AVR_MEGA2560)
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_MASK _BV(7)
#elif defined(ARDUINO_AVR_FEATHER32U4)
#define LED_DDR  DDRC
#define LED_PORT PORTC
#define LED_MASK _BV(7)
#else
#error "Power LED macros are undefined."
#endif

namespace {

inline void led_init() { LED_DDR |= LED_MASK; }
inline void led_on() { LED_PORT |= LED_MASK; }
inline void led_off() { LED_PORT &= ~LED_MASK; }

}

namespace Power {

void Init() {
  // Light the LED. It will indicate if the CPU is awake.
  led_init();
  led_on();

  // Disable the analog comparator.
  ACSR = _BV(ACD);

  // Disable other unused functions.
#define DISABLE 1
#define ENABLE  0
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

void SleepIdle() {
  led_off();
#ifdef UDIEN
  // If not disabled, USB Start-Of-Frame interrupts will immediately wake up
  // the CPU.
  UDIEN &= ~_BV(SOFE);
#endif
  // If not disabled, timer/counter 0 interrupts will immediately wake up the
  // CPU.
  PRR0 |= _BV(PRTIM0);
  SMCR = _BV(SE);
  __asm__ __volatile__(
      "sleep"
      "\n\t" ::);
  // This will occur once the CPU is awoken by an interrupt.
  SMCR = 0;
  // Reenable the quashed interrupts.
  PRR0 &= ~_BV(PRTIM0);
#ifdef UDIEN
  UDIEN |= _BV(SOFE);
#endif
  led_on();
}

}  // namespace Power