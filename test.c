#include "test.h"
#include "printf.h"

#define INCLUDE_TESTS
#ifdef INCLUDE_TESTS

#include "string.h"

static const char *test_help =
"Usage: test <suite>\n"
"  Test Suites\n"
"    printf - Compares output of snprintf to expected data and prints\n"
"             mismatches for debugging.\n"
"";

static void test_printf(void) {
   char expected[80];
   char actual[80];
   int n, e, failures = 0;

#define TEST_PRINTF(c, e, a) \
   do { \
      if (strcmp(e, a) != 0) { \
         printf("FAILED %d\n  Expected: %s\n  Actual:   %s\n", c, e, a); \
         failures++; \
      } \
   } while (0)

   /* Basic conversions (s, c, d, i, o, u, x, X, n) */
   snprintf(actual, 80, "%s %c %d %i %o %u %x %X%n", "test", '~', 42, 42, 42,
         42, 42, 42, &n);
   e = snprintf(expected, 80, "test ~ 42 42 52 42 2a 2A");
   TEST_PRINTF(1, expected, actual);
   if (e != n) {
      printf("FAILED 2\n  \'n\' conversion produced %d instead of %d\n", n, e);
      failures++;
   }

   /* Field width, zero padding, and alignment */
   snprintf(actual, 80, "%*d %04x %2s %-6s!", 4, 42, 42, "hello", "test");
   snprintf(expected, 80, "  42 002a hello test  !");
   TEST_PRINTF(3, expected, actual);

   /* Precision, sign, percent */
   snprintf(actual, 80, "%.4s % d %+d %d%%", "testing", 42, 42, -42);
   n = snprintf(expected, 80, "test  42 +42 -42");
   expected[n++] = '%';
   expected[n] = '\0';
   TEST_PRINTF(4, expected, actual);

   /* Modifiers */
   snprintf(actual, 80, "%hhd %hd %ld %lld %zu", (char)42, (short)42, (long)42,
         (long long)42, (size_t)42);
   snprintf(expected, 80, "42 42 42 42 42");
   TEST_PRINTF(5, expected, actual);

   if (failures == 0)
      printf("PASSED\n");
}

void test_cmd(int argc, const char *argv[]) {
   if (argc != 2)
      goto print_help;

   if (strcmp(argv[1], "printf") == 0) {
      test_printf();
      return;
   }

print_help:
   printf(test_help);
}

#else

void test_cmd(int argc, const char *argv[]) {
   printf("This command is unavailable.\n");
}

#endif
