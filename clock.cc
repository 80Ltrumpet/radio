#include "clock.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace {
volatile unsigned long long millis{};
}

namespace Clock {

void Init() {
  TCNT1 = 0;
  // Output compare
  OCR1A = 250;
  // 1/64 prescale; clear timer on compare match
  TCCR1B = (3 << CS10) | _BV(WGM12);
  TCCR1A = _BV(COM1A0);
  TIMSK1 = _BV(OCIE1A);
}

unsigned long long GetMillis() { return millis; }

}  // namespace Clock

ISR(TIMER1_COMPA_vect) { ++millis; }