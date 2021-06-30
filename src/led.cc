#include "led.h"

#include <avr/io.h>

Gpio Led::gpio_ {
#if defined(ARDUINO_AVR_MEGA2560)
  PINB, 7
#elif defined(ARDUINO_AVR_FEATHER32U4)
  PINC, 7
#else
#error "Unknown board type."
#endif
};

void Led::Init(bool start_on) {
  start_on ? On() : Off();
  gpio_.out();
}