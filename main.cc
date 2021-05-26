#include <avr/interrupt.h>

#include "console.h"
#include "scheduler.h"
#include "timer.h"
#include "usb.h"

int main() {
   Timer::Init();
   Usb::Init();

   sei();
   
   Console::Init();
   Scheduler::Run();

   return 0;
}
