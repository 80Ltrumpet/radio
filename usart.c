#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/setbaud.h>

#include "usart.h"
#include "timer.h"

#define USART_BUF_SIZE     256
#define USART_TX_TIMEOUT   50

typedef struct {
   volatile unsigned int head;
   volatile unsigned int tail;
   char buf[USART_BUF_SIZE];
} ring_buf_t;

static ring_buf_t rx;
static ring_buf_t tx;

void usart_init(void) {
   /* Set up USART0 */
   /* These values are from util/setbaud.h. BAUD is defined in makefile. */
   UBRR0H = UBRRH_VALUE;
   UBRR0L = UBRRL_VALUE;
   /* Double baudrate */
#if USE_2X
   UCSR0A = _BV(U2X0);
#endif
   /* RX complete interrupt, receive, and transmit */
   UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
   /* 8-bit, no parity, 1 stop bit */
   UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

   /* Initialize ring buffers */
   rx.head = rx.tail = 0;
   tx.head = tx.tail = 0;
}

char usart_getc(void) {
   char c = 0;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (rx.tail <= rx.head)
         rx.head = rx.tail = 0;
      else
         c = rx.buf[rx.head++ & (USART_BUF_SIZE - 1)];
   }

   return c;
}

int usart_putc(char c) {
   unsigned long long timeout;
   /* Precede all newlines with carriage returns */
   if (c == '\n' && usart_putc('\r') == 0)
      return 0;

   /* Wait until there is room in the TX buffer */
   timeout = ticks + USART_TX_TIMEOUT;
   while (tx.tail - tx.head >= USART_BUF_SIZE)
      if (ticks >= timeout)
         return 0;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      tx.buf[tx.tail++ & (USART_BUF_SIZE - 1)] = c;
      UCSR0B |= _BV(UDRIE0);
      UCSR0A |= _BV(TXC0);
   }

   return 1;
}

int usart_putcn(char c, int n) {
   int i;
   for (i = 0; i < n && usart_putc(c); i++)
      ;
   return i;
}

void usart_puts(const char *s) {
   while (*s != '\0' && usart_putc(*s++))
      ;
}

ISR(USART0_RX_vect) {
   unsigned char parity_check = bit_is_clear(UCSR0A, UPE0);
   char c = UDR0;
   if (parity_check) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
         /* Drop input that would overflow the buffer */
         if (rx.tail - rx.head < USART_BUF_SIZE)
            rx.buf[rx.tail++ & (USART_BUF_SIZE - 1)] = c;
      }
   }
}

ISR(USART0_UDRE_vect) {
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (tx.head < tx.tail) {
         UDR0 = tx.buf[tx.head++ & (USART_BUF_SIZE - 1)];
      } else {
         /* Nothing to transmit. Disable this interrupt and reset the buffer. */
         UCSR0B &= ~_BV(UDRIE0);
         tx.head = tx.tail = 0;
      }
   }
}
