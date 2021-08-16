#include <avr/interrupt.h>

#include "console.h"
#include "led.h"
#include "platform.h"
#include "power.h"
#include "radio.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"
#include "twi.h"

int main() {
  Power::Init();

  Led::Init(true);
  Spi::Init();
  Twi::Init();
  Timer::Init();
  Platform::InitPreSei();

  sei();

  Console::Init();
  Radio::Init();
  Platform::InitPostSei();

  Scheduler::Run();
}
