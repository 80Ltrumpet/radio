#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

extern volatile unsigned long long ticks;

void timer_init(void);
void timer_cmd(int argc, const char *argv[]);

#endif
