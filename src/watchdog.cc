#include "watchdog.h"

#include <avr/wdt.h>

namespace Watchdog {

enum class Mode {
  Off,
  Paused,
  Normal,
  HighPriority,
};

auto mode_{Mode::Off};
auto non_hp_mode_{Mode::Off};
uint8_t wdtcsr_{};

void resume_impl() {
  wdt_reset();
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  WDTCSR = wdtcsr_;
}

void Init() { wdt_disable(); }

void Start() {
  if (mode_ == Mode::Off) {
    mode_ = Mode::Normal;
    wdt_enable(WDTO_60MS);
  }
}

void StartHighPriority() {
  if (mode_ != Mode::HighPriority) {
    non_hp_mode_ = mode_;
    mode_ = Mode::HighPriority;
    wdtcsr_ = WDTCSR;
    wdt_enable(WDTO_120MS);
  }
}

void StopHighPriority() {
  if (mode_ == Mode::HighPriority) {
    mode_ = non_hp_mode_;
    resume_impl();
  }
}

void Pause() {
  if (mode_ == Mode::Normal) {
    mode_ = Mode::Paused;
    wdtcsr_ = WDTCSR;
    wdt_disable();
  }
}

void Resume() {
  if (mode_ == Mode::Paused) {
    mode_ = Mode::Normal;
    resume_impl();
  }
}

void Kick() {
  if (mode_ == Mode::Normal) wdt_reset();
}

}  // namespace Watchdog