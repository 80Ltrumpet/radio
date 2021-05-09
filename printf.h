#ifndef _PRINTF_H
#define _PRINTF_H

#include <stddef.h>
#include <stdarg.h>

/*
 * For some odd reason, we cannot name the function "printf" and call it
 * directly in some cases. In those cases, the call to printf is missing
 * entirely in the compiled output.
 */
#define printf(fmt, ...) uprintf(fmt, ##__VA_ARGS__)
int uprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));

int snprintf(char *str, size_t size, const char *format, ...)
   __attribute__((format(printf, 3, 4)));

int vprintf(const char *format, va_list ap)
   __attribute__((format(printf, 1, 0)));
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
   __attribute__((format(printf, 3, 0)));

#endif
