#include <avr/interrupt.h>

#include "cmdline.h"
#include "sched.h"
#include "spi.h"
#include "timer.h"
#include "usart.h"
#include "w5100.h"

int main(void) {
   Spi::Init();
   timer_init();
   usart_init();

   sei();

   w5100_init();
   cmdline_init();

   while (1)
      scheduler();

   return 0;
}
