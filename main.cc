#include <avr/interrupt.h>

#include "clock.h"
#include "console.h"
#include "scheduler.h"

int main() {
   Clock::Init();
   Console::Init();

   sei();

   Scheduler::Run();

   return 0;
}
