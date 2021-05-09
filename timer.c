#include <avr/io.h>
#include <avr/interrupt.h>

#include "printf.h"
#include "string.h"

#define TIMER_MAX_CONFIGS     5
#define TIMER_F_TICK_DEFAULT  1000UL

/*
 * These clock shifts correspond to the oscillator prescaler values defined
 * in the ATmega2560 datasheet (1/1, 1/8, 1/16, 1/64, 1/256, 1/1024).
 */
static const unsigned int clock_shifts[TIMER_MAX_CONFIGS] = { 0, 3, 6, 8, 10 };

/* Use timer/counter 1 for the tick counter. */
volatile unsigned long long ticks;

static struct {
   unsigned long f_tick;
   unsigned int n_cfg;
   unsigned int sel_cfg;
   struct {
      unsigned long oc;
      long err;
      long err_dec;
      int err_prec;
      uint8_t cs;
   } configs[TIMER_MAX_CONFIGS];
} timers[2];

static unsigned int active_timer;

static unsigned int get_configs_for_proposed_freq(unsigned long freq) {
   unsigned int i, cs, n = 0, sel = 0;
   unsigned int prop_timer = 1 - active_timer;
   unsigned long oc;
   long err, err_div;
   int err_prec;

   for (i = 0; i < TIMER_MAX_CONFIGS; i++) {
      cs = clock_shifts[i];
      if (!(F_CPU & (_BV(cs) - 1)) && (F_CPU >> cs) > freq) {
         oc = (F_CPU >> cs) / freq;
         if (oc > UINT16_MAX)
            continue; // Won't fit in output compare register

         err_prec = 0;
         err_div = 100;
         err = ((F_CPU * err_div) >> cs);
         if (_BV(cs) > 1000) {
            err *= 1000;
            err_div *= 1000;
            err_prec += 3;
         } else if (_BV(cs) > 100) {
            err *= 100;
            err_div *= 100;
            err_prec += 2;
         } else if (_BV(cs) > 10) {
            err *= 10;
            err_div *= 10;
            err_prec += 1;
         }
         err /= oc;
         if (oc > 10000) {
            err *= 10000;
            err_div *= 10000;
            err_prec += 4;
         } else if (oc > 1000) {
            err *= 1000;
            err_div *= 1000;
            err_prec += 3;
         } else if (oc > 100) {
            err *= 100;
            err_div *= 100;
            err_prec += 2;
         } else if (oc > 10) {
            err *= 10;
            err_div *= 10;
            err_prec += 1;
         }
         err = (err - (long)freq * err_div) / (long)freq;
         err_div /= 100;

         timers[prop_timer].configs[n].oc       = oc;
         timers[prop_timer].configs[n].err      = err / err_div;
         timers[prop_timer].configs[n].err_dec  = (err < 0 ? -err : err) % err_div;
         timers[prop_timer].configs[n].err_prec = err_prec;
         timers[prop_timer].configs[n].cs     = (i + 1) << CS10;

         /*
          * Compare integer and decimal parts of the error to determine if this
          * configuration is better than the currently selected one.
          */
         if (n != sel) {
            if (timers[prop_timer].configs[n].err < timers[prop_timer].configs[sel].err)
               sel = n;
            else if (timers[prop_timer].configs[n].err == timers[prop_timer].configs[sel].err) {
               int prec_diff = timers[prop_timer].configs[n].err_prec -
                  timers[prop_timer].configs[sel].err_prec;
               long n_dec = timers[prop_timer].configs[n].err_dec;
               long sel_dec = timers[prop_timer].configs[sel].err_dec;
               long n_mul = 1, sel_mul = 1;
               while (prec_diff > 0) {
                  sel_mul *= 10;
                  prec_diff--;
               }
               while (prec_diff < 0) {
                  n_mul *= 10;
                  prec_diff++;
               }
               n_dec *= n_mul;
               sel_dec *= sel_mul;
               if (n_dec <= sel_dec)
                  sel = n;
            }
         }

         n++;
      }
   }

   if (n > 0) {
      timers[prop_timer].f_tick  = freq;
      timers[prop_timer].n_cfg   = n;
      timers[prop_timer].sel_cfg = timers[active_timer].n_cfg + sel;
   } else {
      timers[prop_timer].f_tick  = 0;
      timers[prop_timer].n_cfg   = 0;
      timers[prop_timer].sel_cfg = timers[active_timer].sel_cfg;
   }

   return n;
}

static int apply_proposed_config(void) {
   unsigned int prop_timer = 1 - active_timer;
   unsigned int sel_cfg = timers[prop_timer].sel_cfg;
   int use_new_f_tick = sel_cfg >= timers[active_timer].n_cfg;

   if ((use_new_f_tick && timers[prop_timer].f_tick == 0) ||
       sel_cfg == timers[active_timer].sel_cfg)
      return 0;

   /* Stop the timer */
   TIMSK1 = 0;
   TCCR1A = 0;

   /* Reset ticks */
   ticks = 0;

   /* Reconfigure timer */
   TCNT1 = 0;
   if (use_new_f_tick) {
      sel_cfg -= timers[active_timer].n_cfg;
      timers[prop_timer].sel_cfg = sel_cfg;
      OCR1A = (uint16_t)(timers[prop_timer].configs[sel_cfg].oc);
      TCCR1B = timers[prop_timer].configs[sel_cfg].cs | _BV(WGM12);
   } else {
      timers[active_timer].sel_cfg = sel_cfg;
      OCR1A = (uint16_t)(timers[active_timer].configs[sel_cfg].oc);
      TCCR1B = timers[active_timer].configs[sel_cfg].cs | _BV(WGM12);
   }
   TCCR1A = _BV(COM1A0);
   TIMSK1 = _BV(OCIE1A);

   if (use_new_f_tick) {
      timers[active_timer].f_tick = 0;
      active_timer = prop_timer;
   }

   return 1;
}

static inline int cs_bits_to_prescale(uint8_t cs) {
   return 1 << clock_shifts[(cs >> CS10) - 1];
}

static void show_timer_info(void) {
   unsigned int i, j, prop_timer = 1 - active_timer;

   printf("Tick count: %llu\n", ticks);
   printf("Current tick frequency: %lu Hz\n\n", timers[active_timer].f_tick);
   printf("  ID  PRSC  OCMP  Error (%%)\n");
   printf("  --  ----  ----  ---------\n");
   for (i = 0; i < timers[active_timer].n_cfg; i++)
      printf("%c %2x  %4d  %4x  %ld.%0*ld\n",
            i == timers[active_timer].sel_cfg ? '-' : ' ', i,
            cs_bits_to_prescale(timers[active_timer].configs[i].cs),
            (uint16_t)(timers[active_timer].configs[i].oc),
            timers[active_timer].configs[i].err,
            timers[active_timer].configs[i].err_prec,
            timers[active_timer].configs[i].err_dec);
   printf("\n");

   if (timers[prop_timer].f_tick == 0)
      return;
   printf("Proposed tick frequency: %lu Hz\n\n", timers[prop_timer].f_tick);
   if (timers[prop_timer].n_cfg == 0) {
      printf("  No configurations exist for the proposed tick frequency!\n");
      return;
   }
   printf("  ID  PRSC  OCMP  Error (%%)\n");
   printf("  --  ----  ----  ---------\n");
   for (j = 0; j < timers[prop_timer].n_cfg; j++, i++)
      printf("%c %2x  %4d  %4x  %ld.%0*ld\n",
            i == timers[prop_timer].sel_cfg ? '*' : ' ', i,
            cs_bits_to_prescale(timers[prop_timer].configs[j].cs),
            (uint16_t)(timers[prop_timer].configs[j].oc),
            timers[prop_timer].configs[j].err,
            timers[prop_timer].configs[j].err_prec,
            timers[prop_timer].configs[j].err_dec);
   printf("\n");
}

void timer_init(void) {
   active_timer = 1;
   timers[active_timer].f_tick = 0;
   timers[active_timer].n_cfg = 0;
   get_configs_for_proposed_freq(TIMER_F_TICK_DEFAULT);
   apply_proposed_config();
}

void timer_cmd(int argc, const char *argv[]) {
   unsigned int prop_timer = 1 - active_timer;

   if (argc < 2) {
print_help:
      printf("Usage: timer info\n"
             "             freq <tick frequency>\n"
             "             select <id>\n"
             "             revert\n"
             "             apply\n");
      return;
   }

   if (strcmp("info", argv[1]) == 0) {
      show_timer_info();
   } else if (strcmp("apply", argv[1]) == 0) {
      if (!apply_proposed_config())
         printf("No new configuration has been selected.\n");
      show_timer_info();
   } else if (strcmp("revert", argv[1]) == 0) {
      timers[prop_timer].f_tick = 0;
      show_timer_info();
   } else if (strcmp("freq", argv[1]) == 0) {
      unsigned long f;
      if (argc != 3)
         goto print_help;
      if (strtoul(argv[2], &f) != 0) {
         printf("\"%s\" is not a decimal frequency.\n", argv[2]);
         goto print_help;
      }
      if (f == timers[active_timer].f_tick) {
         printf("Timer is already operating at %lu Hz.\n", f);
         return;
      }
      if (get_configs_for_proposed_freq(f) == 0) {
         printf("%lu Hz is not achievable with this timer.\n", f);
         return;
      }
      show_timer_info();
   } else if (strcmp("select", argv[1]) == 0) {
      unsigned int c;
      if (argc != 3)
         goto print_help;
      if (strtohex(argv[2], &c) != 0) {
         printf("\"%s\" is not a hexadecimal configuration ID.\n", argv[2]);
         goto print_help;
      } else if (c >= timers[active_timer].n_cfg + timers[prop_timer].n_cfg)
         printf("Configuration %x does not exist.\n", c);
      else
         timers[prop_timer].sel_cfg = c;
      show_timer_info();
   } else {
      goto print_help;
   }
}

ISR(TIMER1_COMPA_vect) {
   ticks++;
}
