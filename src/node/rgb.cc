#include "rgb.h"

#include <math.h>

#include "aw9523b.h"
#include "eeprom.h"
#include "scheduler.h"
#include "timer.h"
#include "twi.h"

using namespace aw9523b;

namespace {

using Rgb::Color;
using Rgb::Pattern;

constexpr uint8_t kMaxRgbCount{5};      // 16 / 3
constexpr uint16_t kMinPeriodMs{200};   // 5 Hz
constexpr uint16_t kAnimStepMs{40};     // 25 Hz

Twi::Device dev_{kI2cAddr};
TaskHandle task_{};

// Each RGB group must be interlaced by LED color in the hardware
// configuration. Where N = rgb_count_:
//  Port [0,   N-1] = Red   [0, N-1]
//  Port [N,  2N-1] = Green [0, N-1]
//  Port [2N, 3N-1] = Blue  [0, N-1]
uint8_t rgb_count_{};

// Use the same color for everything.
Color color_{};

// Remember the previous color for transition interpolation.
Color prev_color_{};

// Color most recently sent to the LED driver
Color cur_color_{};

Pattern pattern_{Pattern::None};

uint64_t period_start_ms_{};
uint16_t period_ms_{kMinPeriodMs};

uint64_t transition_start_ms_{};
uint16_t transition_period_ms_{};

// Linearly interpolates from a to b where t is in [0, 1].
uint8_t lerp(uint8_t a, uint8_t b, float t) {
  return a < b ? a + static_cast<uint8_t>(t * (b - a))
               : a - static_cast<uint8_t>(t * (a - b));
}

// Performs component-wise linear interpolation from a to b where t is in
// [0, 1].
Color color_lerp(const Color& a, const Color& b, float t) {
  return Color{lerp(a.r, b.r, t), lerp(a.g, b.g, t),
               lerp(a.b, b.b, t)};
}

// Sets all RGB LEDs to the given color.
void update(const Color& color) {
  const uint8_t led_count{Color::N * rgb_count_};
  uint8_t dim[led_count];
  for (uint8_t vi{}; vi < Color::N; ++vi) {
    for (uint8_t rgbi{}; rgbi < rgb_count_; ++rgbi) {
      dim[rgbi + vi * rgb_count_] = color.v[vi];
    }
  }

  dev_.write(Reg::DIM(0), dim, led_count);
  cur_color_ = color;
}

bool handle_transition(uint64_t now) {
  // Check if we are in a transition.
  if (transition_start_ms_ == 0) {
    return false;
  }

  // Check if the transition period is over.
  if (now > transition_start_ms_ + transition_period_ms_) {
    transition_start_ms_ = period_start_ms_ = 0;
    return false;
  }

  // Otherwise, perform a linear interpolation.
  auto t{(now - transition_start_ms_) /
         static_cast<float>(transition_period_ms_)};
  update(color_lerp(prev_color_, color_, t));
  return true;
}

void run() {
  const auto now{Timer::Millis()};
  if (handle_transition(now)) {
    return;
  }

  // Handle the first update after the transition.
  if (period_start_ms_ == 0) {
    period_start_ms_ = now;
    // Set pattern schedule.
    switch (pattern_) {
      case Pattern::None:
        task_->pause();
        break;
      case Pattern::Blink:
        task_->set_period(period_ms_ >> 1);
        break;
      default:
        task_->set_period(kAnimStepMs);
    }
    update(color_);
    return;
  }

  if (pattern_ == Pattern::None) {
    // This should never execute.
    task_->pause();
    return;
  }

  // Blink relies on the scheduler and simply toggles the color.
  if (pattern_ == Pattern::Blink) {
    update(cur_color_ ? Color{} : color_);
    return;
  }

  // Compute t.
  const auto duration{now - period_start_ms_};
  const uint16_t period_count{duration / period_ms_};
  const auto t{duration / static_cast<float>(period_ms_) - period_count};

  // Compute s (sinusoidal interpolation coefficient).
  const auto s{cosf(2 * M_PI * t) * 0.5f + 0.5f};

  switch (pattern_) {
    case Pattern::Throb:
      update(color_lerp(Color{color_.r >> 2, color_.g >> 2, color_.b >> 2},
                        color_, s));
      break;
    case Pattern::SineOff:
      update(color_lerp(Color{}, color_, s));
      break;
    case Pattern::SineWhite:
      update(color_lerp(Color{255, 255, 255}, color_, s));
      break;
    default:
      // Should never get here.
      break;
  }
}

}  // namespace

namespace Rgb {

void Init() {
  // Check if the driver is connected.
  if (dev_.read(Reg::ID) != kId) {
    return;
  }

  Eeprom::Read(Eeprom::Data::RgbCount, &rgb_count_);
  if (rgb_count_ == 0 || rgb_count_ > kMaxRgbCount) {
    return;
  }

  // Determine the word-register mask for the LEDs.
  const uint8_t led_count{Color::N * rgb_count_};
  const auto led_mask{WordRegMask((1 << led_count) - 1)};

  // Set the maximum current to 19 mA and configure LED ports.
  uint8_t buf[3]{
      Bits::ISEL_19MA,      // CTL
      ~(led_mask & 0xff),   // MODE0
      ~(led_mask >> 8)      // MODE1
  };
  dev_.write(Reg::CTL, buf, sizeof(buf));

  task_ = Scheduler::AddTask({"rgb", run, 0, Task::kPause});
}

void Clear() {
  Mutator{}
      .set_color({})
      .set_pattern(Pattern::None)
      .set_period(0)
      .set_transition_period(0);
}

Mutator::Mutator() {
  if (task_) {
    task_->set_period(kAnimStepMs);
    task_->start();
    transition_start_ms_ = Timer::Millis();
  }
}

const Mutator& Mutator::set_color(const Color& color) const {
  prev_color_ = cur_color_;
  color_ = color;
  return *this;
}

const Mutator& Mutator::set_color(Color&& color) const {
  prev_color_ = cur_color_;
  color_ = static_cast<Color&&>(color);
  return *this;
}

const Mutator& Mutator::set_pattern(Pattern pattern) const {
  pattern_ = pattern;
  return *this;
}

const Mutator& Mutator::set_period(uint16_t period_ms) const {
  period_ms_ = period_ms > kMinPeriodMs ? period_ms : kMinPeriodMs;
  return *this;
}

const Mutator& Mutator::set_transition_period(uint16_t transition_ms) const {
  transition_period_ms_ = transition_ms;
  return *this;
}

}  // namespace Rgb