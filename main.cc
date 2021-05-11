#include <avr/interrupt.h>

#include "console.h"
#include "timer.h"

int main() {
   Timer::Init();
   Console::Init();

   sei();

   for (;;) {
      //scheduler();
   }

   return 0;
}
