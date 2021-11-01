#include "relay.h"

#include <avr/io.h>

#include "gpio.h"
#include "scheduler.h"

enum class RunMode {
  None,
  Off,
  FlickerOn,
};

namespace {

Gpio control_{PINE, 4};
TaskHandle task_{};
auto run_mode_{RunMode::None};
uint8_t flicker_count_{};

void run_off() {
  control_.set();
  task_->pause();
  run_mode_ = RunMode::None;
}

void run_flicker_on() {
  constexpr uint8_t kFlickerEnd{2 * 5};
  constexpr uint16_t kInitialFlickerPeriod{50};
  constexpr uint16_t kFlickerPeriodIncrement{25};

  if (flicker_count_ < kFlickerEnd) {
    if (flicker_count_++ & 1) {
      control_.set();
    } else {
      control_.clear();
    }
    task_->start((flicker_count_ >> 1) * kFlickerPeriodIncrement +
                  kInitialFlickerPeriod);
  } else {
    control_.clear();
    run_mode_ = RunMode::None;
    task_->pause();
  }
}

void run() {
  switch (run_mode_) {
  case RunMode::Off:
    run_off();
    break;
  case RunMode::FlickerOn:
    run_flicker_on();
    break;
  default:
    task_->pause();
    break;
  }
}

}  // namespace

namespace Relay {

void Init() {
  // The relay is active low.
  control_.set();
  control_.out();

  task_ = Scheduler::AddTask({"relay", run, 0, Task::kPause});
}

void SwitchOn() { control_.clear(); }

void SwitchOff(uint16_t delay_ms) {
  run_mode_ = RunMode::Off;
  task_->start(delay_ms);
}

void FlickerOn() {
  run_mode_ = RunMode::FlickerOn;
  flicker_count_ = 0;
  task_->start();
}

bool IsOn() { return !control_.is_set(); }

}  // namespace Relay