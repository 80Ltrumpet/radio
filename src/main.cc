#include <avr/interrupt.h>

#include "console.h"
#include "radio.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"
#include "usb.h"

int main() {
   Spi::Init();
   Timer::Init();
   Usb::Init();

   sei();
   
   Console::Init();
   Radio::Init();

   Scheduler::Run();
}
