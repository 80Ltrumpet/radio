#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

size_t strlen(const char *s);

#define strncpy(d, s, n) ustrncpy(d, s, n)
char * ustrncpy(char *dst, const char *src, size_t n);
void * memcpy(void *dst, const void *src, size_t n);

void * bzero(void *buf, size_t n);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

int stricmp(const char *a, const char *b);
int strincmp(const char *a, const char *b, size_t n);

int strtohex(const char *s, unsigned int *v);
int strtoul(const char *s, unsigned long *v);

#endif
