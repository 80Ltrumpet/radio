#include <avr/interrupt.h>

#include "console.h"
#include "led.h"
#include "network.h"
#include "power.h"
#include "radio.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"
#include "usart.h"
#include "usb.h"

int main() {
  Power::Init();

  Led::Init();
  Spi::Init();
  Timer::Init();
  Usart::Init();
  Usb::Init();

  sei();

  Console::Init();
  Radio::Init();
  Network::Init();

  Scheduler::Run();
}
