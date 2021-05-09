#include "string.h"

static inline char lowercase(char c) {
   return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

size_t strlen(const char *s) {
   size_t len = 0;
   while (*s++ != '\0')
      len++;
   return len;
}

char * ustrncpy(char *dst, const char *src, size_t n) {
   char *ret = dst;
   for (; *src != '\0' && n > 0; n--)
      *dst++ = *src++;
   return ret;
}

void * memcpy(void *dst, const void *src, size_t n) {
   void *ret = dst;
   char *d = (char *)dst, *s = (char *)src;
   for (; n > 0; n--)
      *d++ = *s++;
   return ret;
}

void * bzero(void *buf, size_t n) {
   char *b = (char *)buf;
   for (; n > 0; n--)
      *b = 0;
   return buf;
}

int strcmp(const char *a, const char *b) {
   while (1) {
      char x = *a++, y = *b++;
      if (x > y)
         return 1;
      if (x < y)
         return -1;
      if (x == '\0')
         break;
   }
   return 0;
}

int strncmp(const char *a, const char *b, size_t n) {
   for (size_t i = 0; i < n; i++) {
      char x = a[i], y = b[i];
      if (x > y)
         return 1;
      if (x < y)
         return -1;
      if (x == '\0')
         break;
   }
   return 0;
}

int stricmp(const char *a, const char *b) {
   while (1) {
      char x = lowercase(*a++), y = lowercase(*b++);
      if (x > y)
         return 1;
      if (x < y)
         return -1;
      if (x == '\0')
         break;
   }
   return 0;
}

int strincmp(const char *a, const char *b, size_t n) {
   for (size_t i = 0; i < n; i++) {
      char x = lowercase(a[i]), y = lowercase(b[i]);
      if (x > y)
         return 1;
      if (x < y)
         return -1;
      if (x == '\0')
         break;
   }
   return 0;
}

int strtohex(const char *s, unsigned int *v) {
   unsigned int value = 0;
   for (char c = *s; c != '\0'; c = *(++s)) {
      value <<= 4;
      if (c >= '0' && c <= '9')
         value |= c - '0';
      else if (c >= 'a' && c <= 'f')
         value |= c - 'a' + 0xa;
      else if (c >= 'A' && c <= 'F')
         value |= c - 'A' + 0xA;
      else
         return -1;
   }
   *v = value;
   return 0;
}

int strtoul(const char *s, unsigned long *v) {
   unsigned long value = 0;
   for (char c = *s; c != '\0'; c = *(++s)) {
      value *= 10;
      if (c >= '0' && c <= '9')
         value += c - '0';
      else
         return -1;
   }
   *v = value;
   return 0;
}
