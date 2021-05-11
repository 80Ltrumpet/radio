#include "timer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>

static constexpr uint8_t kMaxConfigs{6};
static constexpr unsigned long kDefaultTickFrequency{1000UL};
static constexpr uint8_t kNumTimers{2};

// These clock shifts correspond to the supported oscillator prescaler values
// defined in the ATmega2560 data sheet (1/1, 1/8, 1/16, 1/64, 1/256, 1/1024).
static constexpr const uint8_t kClockShift[kMaxConfigs]{0, 3, 4, 6, 8, 10};

// Timer configurations
static struct {
  unsigned long tick_frequency{};
  uint8_t config_count{};
  uint8_t selected_config{};
  struct {
    unsigned long output_compare{};
    long err{};
    long err_decimal{};
    int8_t err_precision{};
    uint8_t clock_shift{};
  } config[kMaxConfigs];
} timer[kNumTimers];

static uint8_t active_timer{1};

static volatile unsigned long long ticks{};

static constexpr uint16_t PrescaleFromClockShift(uint8_t cs) {
  return 1 << kClockShift[(cs >> CS10) - 1];
}

static uint8_t GetConfigsForProposedTickFrequency(unsigned long freq) {
  auto& proposed{timer[1 - active_timer]};
  proposed.config_count = 0;
  proposed.selected_config = 0;

  for (uint8_t i{0}; i < kMaxConfigs; ++i) {
    // If the prescaler does not evenly divide the clock or the scaled clock
    // frequency is lower than the requested frequency, it cannot be used.
    const auto cs{kClockShift[i]};
    if ((F_CPU & (_BV(cs) - 1)) || (F_CPU >> cs) <= freq) {
      continue;
    }

    // Check if the scaled clock counts will fit in the output compare register
    // for the requested frequency.
    auto& config{proposed.config[proposed.config_count]};
    config.output_compare = (F_CPU >> cs) / freq;
    if (config.output_compare > UINT16_MAX) {
      continue;
    }

    int8_t m{3};    // "Magnitude" for precision determination
    long f{1000L};  // "Factor" for divisor determination
    long err_div{100L};
    config.err_precision = 0;
    config.err = (F_CPU * err_div) >> cs;

    for (; m > 0 && _BV(cs) <= f; --m, f /= 10L);
    err_div *= f;
    config.err_precision += m;
    config.err = (config.err * f) / config.output_compare;

    for (m = 4, f = 10000UL;
         m > 0 && config.output_compare <= f;
         --m, f /= 10L);
    err_div *= f;
    config.err_precision += m;
    config.err = (config.err * f - freq * err_div) / static_cast<long>(freq);

    // Reset the divisor and finish populating the configuration.
    err_div /= 100L;
    config.err /= err_div;
    config.err_decimal = labs(config.err) % err_div;
    config.clock_shift = (i + 1) << CS10;

    // Compare the integral and decimal parts of the error to determine if this
    // configuration is better than the currently-selected one.
    const auto& selected_config{proposed.config[proposed.selected_config]};
    if (proposed.config_count == proposed.selected_config) {
      // There is only one configuration, so far.
    } else if (config.err < selected_config.err) {
      // Use the new configuration.
      proposed.selected_config = proposed.config_count;
    } else if (config.err == selected_config.err) {
      // Check the decimal parts.
      auto prec_diff{config.err_precision - selected_config.err_precision};
      auto dec_new{config.err_decimal};
      auto dec_sel{selected_config.err_decimal};
      for (; prec_diff > 0L; --prec_diff) {
        dec_sel *= 10L;
      }
      for (; prec_diff < 0L; ++prec_diff) {
        dec_new *= 10L;
      }
      if (dec_new <= dec_sel) {
        proposed.selected_config = proposed.config_count;
      }
    }

    ++proposed.config_count;
  }

  if (proposed.config_count > 0) {
    proposed.tick_frequency = freq;
    proposed.selected_config += timer[active_timer].config_count;
  } else {
    proposed.tick_frequency = 0UL;
    proposed.selected_config = timer[active_timer].selected_config;
  }

  return proposed.config_count;
}

static bool ApplyProposedConfig() {
  auto& active{timer[active_timer]};
  auto& proposed{timer[1 - active_timer]};
  auto use_new_freq{proposed.selected_config >= active.config_count};
  if ((use_new_freq && proposed.tick_frequency == 0) ||
      proposed.selected_config == active.selected_config) {
    return false;
  }

  // Stop the timer.
  TIMSK1 = 0;
  TCCR1A = 0;

  // Reset ticks.
  ticks = 0;

  // Reconfigure the timer based on the proposed configuration.
  TCNT1 = 0;
  if (use_new_freq) {
    proposed.selected_config -= active.config_count;
    const auto& config{proposed.config[proposed.selected_config]};
    OCR1A = static_cast<uint16_t>(config.output_compare);
    TCCR1B = config.clock_shift | _BV(WGM12);

    active.tick_frequency = 0;
    active_timer = 1 - active_timer;
  } else {
    active.selected_config = proposed.selected_config;
    const auto& config{active.config[active.selected_config]};
    OCR1A = static_cast<uint16_t>(config.output_compare);
    TCCR1B = config.clock_shift | _BV(WGM12);
  }

  return true;
}

template <class T>
static void DumpTimerConfigs(const T& timer) {
  puts("  ID  PRSC  OCMP  Error (%%)");
  puts("  --  ----  ----  ---------");
  for (uint8_t i{0}; i < timer.config_count; ++i) {
    const auto& config{timer.config[i]};
    printf("%c %2x  %4" PRId16 "  %4x  %ld.%0*ld\n",
           i == timer.selected_config ? '*' : ' ', i,
           PrescaleFromClockShift(config.clock_shift), config.output_compare,
           config.err, config.err_precision, config.err_decimal);
  }
  puts("");
}

static void DumpTimers() {
  const auto& active{timer[active_timer]};
  printf("Tick count: %llu\n", ticks);
  printf("Current tick frequency: %lu Hz\n\n", active.tick_frequency);
  DumpTimerConfigs(active);

  const auto& proposed{timer[1 - active_timer]};
  if (proposed.tick_frequency == 0) return;
  printf("Proposed tick frequency: %lu Hz\n\n", proposed.tick_frequency);
  if (proposed.config_count == 0) {
    puts("  No configurations exist!");
    return;
  }
  DumpTimerConfigs(proposed);
}

const char* const Timer::kCommandName{"timer"};

void Timer::Init() {
  GetConfigsForProposedTickFrequency(kDefaultTickFrequency);
  ApplyProposedConfig();
}

inline volatile unsigned long long Timer::GetTicks() {
  return ticks;
}

void Timer::Execute(int argc, const char* argv[]) {
  // TODO: Implement these.
  puts("Usage: timer apply\n"
       "             freq <hz>\n"
       "             info\n"
       "             revert\n"
       "             select <id>");
}

ISR(TIMER1_COMPA_vect) {
  ++ticks;
}