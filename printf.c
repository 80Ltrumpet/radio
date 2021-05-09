#include "printf.h"
#include "usart.h"

#define PRINTF_MAX_CONV_LEN   32
#define PRINTF_CONV_LAST      (PRINTF_MAX_CONV_LEN - 1)

typedef enum {
   TYPE_INT,
   TYPE_CHAR,
   TYPE_SHORT,
   TYPE_LONG,
   TYPE_LONG_LONG,
   TYPE_SIZE,
} type_t;

typedef enum {
   ALIGN_RIGHT,
   ALIGN_LEFT,
} align_t;

enum {
   SIGN_MINUS = -1,
   SIGN_NONE,
   SIGN_PLUS,
   SIGN_SPACE,
   SIGN_UNSIGNED,
};

__attribute__((format(printf, 3, 0))) 
static int formatted_print(char *dst, size_t size, const char *format,
      va_list ap) {
   int i = 0;
   char conv[PRINTF_MAX_CONV_LEN];
   char *s;
   int slen;
   long long d;
   unsigned long long u;
   int fw, prec, sign, zero, conv_idx;
   type_t type;
   align_t alignment;

   if (dst != NULL && size == 0)
      goto terminate;

   conv[PRINTF_CONV_LAST] = '\0';

#define PUT(c) \
   do { \
      if (dst == NULL) \
         usart_putc(c); \
      else if (i + 1 < size) \
         dst[i] = (c); \
      i++; \
   } while (0)

   for (char c = *format; c != '\0'; c = *(++format)) {
      if (c != '%') {
         PUT(c);
         continue;
      }

      slen = 0;
      fw = prec = -1;
      zero = 0;
      sign = SIGN_NONE;
      conv_idx = PRINTF_CONV_LAST;
      type = TYPE_INT;
      alignment = ALIGN_RIGHT;

      while ((c = *++format) != '\0') {
         if (c == '%') {
            PUT(c);
            break;
         } else if (c == 's') {
            s = va_arg(ap, char *);
            while (s[slen] != '\0')
               slen++;
            conv_idx = PRINTF_CONV_LAST - slen;
put_conv_no_zero_pad:
            zero = 0;
put_conv:
            slen = fw < 0 ? 0 : fw - (PRINTF_CONV_LAST - conv_idx);
            if (alignment == ALIGN_RIGHT)
               for (int j = 0; j < slen; j++)
                  PUT(zero ? '0' : ' ');
            while (prec-- != 0 && *s) PUT(*s++);
            if (alignment == ALIGN_LEFT)
               for (int j = 0; j < slen; j++)
                  PUT(' ');
            break;
         } else if (c == 'c') {
            conv[--conv_idx] = (char)va_arg(ap, int);
            s = conv + conv_idx;
            goto put_conv_no_zero_pad;
         } else if (c == 'd' || c == 'i') {
put_decimal:
            prec = -1;
            switch (type) {
               default:             d = va_arg(ap, int);       break;
               case TYPE_LONG:      d = va_arg(ap, long);      break;
               case TYPE_LONG_LONG: d = va_arg(ap, long long); break;
            }
            if (sign != SIGN_UNSIGNED && d < 0) {
               d = -d;
               sign = SIGN_MINUS;
            }
            u = (unsigned long long)d;
            do {
               conv[--conv_idx] = '0' + (unsigned int)(u % 10);
               u /= 10;
            } while (u > 0 && conv_idx > 0);
            if (conv_idx > 0) {
               switch (sign) {
                  case SIGN_MINUS:  conv[--conv_idx] = '-'; break;
                  case SIGN_PLUS:   conv[--conv_idx] = '+'; break;
                  case SIGN_SPACE:  conv[--conv_idx] = ' '; break;
                  default: break;
               }
            }
            s = conv + conv_idx;
            goto put_conv;
         } else if (c == 'u') {
            sign = SIGN_UNSIGNED;
            goto put_decimal;
         } else if (c == 'x' || c == 'X') {
            switch (type) {
               default:             u = va_arg(ap, unsigned int);       break;
               case TYPE_LONG:      u = va_arg(ap, unsigned long);      break;
               case TYPE_LONG_LONG: u = va_arg(ap, unsigned long long); break;
               case TYPE_SIZE:      u = va_arg(ap, size_t);             break;
            }
put_hex:
            prec = -1;
            do {
               unsigned int v = (unsigned int)(u & 0xf);
               if (v < 0xa)
                  conv[--conv_idx] = '0' + v;
               else
                  conv[--conv_idx] = (c == 'x' ? 'a' : 'A') + (v - 0xa);
               u >>= 4;
            } while (u > 0 && conv_idx > 0);
            s = conv + conv_idx;
            goto put_conv;
         } else if (c == 'o') {
            switch (type) {
               default:             u = va_arg(ap, unsigned int);       break;
               case TYPE_LONG:      u = va_arg(ap, unsigned long);      break;
               case TYPE_LONG_LONG: u = va_arg(ap, unsigned long long); break;
               case TYPE_SIZE:      u = va_arg(ap, size_t);             break;
            }
            prec = -1;
            do {
               conv[--conv_idx] = '0' + (unsigned int)(u & 7);;
               u >>= 3;
            } while (u > 0 && conv_idx > 0);
            s = conv + conv_idx;
            goto put_conv;
         } else if (c == 'p') {
            void *p = va_arg(ap, void *);
            u = (unsigned int)p;
            goto put_hex;
         } else if (c == 'n') {
            switch (type) {
               default:
               case TYPE_INT: {
                  int *n = va_arg(ap, int *);
                  *n = i;
                  break;
               }
               case TYPE_CHAR: {
                  char *n = va_arg(ap, char *);
                  *n = (char)i;
                  break;
               }
               case TYPE_SHORT: {
                  short *n = va_arg(ap, short *);
                  *n = (short)i;
                  break;
               }
               case TYPE_LONG: {
                  long *n = va_arg(ap, long *);
                  *n = (long)i;
                  break;
               }
               case TYPE_LONG_LONG: {
                  long long *n = va_arg(ap, long long *);
                  *n = (long long)i;
                  break;
               }
            }
            break;
         } else if (c == 'h') {
            type = type == TYPE_SHORT ? TYPE_CHAR : TYPE_SHORT;
         } else if (c == 'l') {
            type = type == TYPE_LONG ? TYPE_LONG_LONG : TYPE_LONG;
         } else if (c == 'z') {
            type = TYPE_SIZE;
         } else if (c == '-') {
            alignment = ALIGN_LEFT;
         } else if (c >= '0' && c <= '9') {
            if (prec < 0) {
               if (fw < 0 && c == '0')
                  zero = 1;
               else
                  fw = 10 * (fw < 0 ? 0 : fw) + (c - '0');
            } else {
               prec = 10 * prec + (c - '0');
            }
         } else if (c == '.') {
            prec = 0;
         } else if (c == '+') {
            sign = SIGN_PLUS;
         } else if (c == ' ' && sign != SIGN_PLUS) {
            sign = SIGN_SPACE;
         } else if (c == '*') {
            d = va_arg(ap, int);
            if (d < 0) {
               alignment = ALIGN_LEFT;
               d = -d;
            }
            if (prec >= 0)
               prec = (int)d;
            else
               fw = (int)d;
         }
      }
   }

   if (dst != NULL) {
      if (i < size) {
terminate:
         dst[i] = '\0';
      } else
         dst[size - 1] = '\0';
   }

   return i;
}

int uprintf(const char *format, ...) {
   int retval;
   va_list ap;

   va_start(ap, format);
   retval = vprintf(format, ap);
   va_end(ap);

   return retval;
}

int snprintf(char *str, size_t size, const char *format, ...) {
   int retval;
   va_list ap;

   va_start(ap, format);
   retval = vsnprintf(str, size, format, ap);
   va_end(ap);

   return retval;
}

int vprintf(const char *format, va_list ap) {
   return formatted_print(NULL, 0, format, ap);
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
   if (str == NULL || size == 0)
      return 0;
   return formatted_print(str, size, format, ap);
}
