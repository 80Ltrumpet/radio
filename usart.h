#ifndef _USART_H
#define _USART_H

#define KEY_HOME        0x01
#define KEY_LEFT        0x02
#define KEY_CANCEL      0x03
#define KEY_DEL         0x04
#define KEY_END         0x05
#define KEY_RIGHT       0x06
#define KEY_BEEP        0x07
#define KEY_BACKSPACE   0x08
#define KEY_TAB         0x09
#define KEY_NL          0x0a
#define KEY_CLEAR_END   0x0b
#define KEY_REDRAW      0x0c
#define KEY_CR          0x0d
#define KEY_DOWN        0x0e
#define KEY_UP          0x10
#define KEY_REDRAW2     0x12
#define KEY_CLEAR_HOME  0x15
#define KEY_LITERAL     0x16
#define KEY_DELETE_WORD 0x17
#define KEY_DELETE_LINE 0x18
#define KEY_ESCAPE      0x1b
#define KEY_DELETE      0x7f

void usart_init(void);
char usart_getc(void);
int usart_putc(char c);
int usart_putcn(char c, int n);
void usart_puts(const char *s);

#endif
