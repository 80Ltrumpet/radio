#pragma once

#include <stdint.h>

namespace Rgb {

struct Color final {
  static constexpr uint8_t N{3};

  constexpr Color() : r{}, g{}, b{} {}
  constexpr Color(uint8_t red, uint8_t green, uint8_t blue)
      : r{red}, g{green}, b{blue} {}

  operator bool() const { return r || g || b; }

  union {
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
    };
    uint8_t v[N];
  };
};

enum class Pattern : uint8_t {
  None,       // Solid color (on/off)
  Blink,      // Full-scale 50% duty cycle square wave
  Throb,      // Smooth oscillation between 50% and 100% of the chosen color
  SineOff,    // Smooth oscillation between off and the chosen color
  SineWhite,  // Smooth oscillation between white and the chosen color
};

struct Mutator final {
  explicit Mutator();

  const Mutator& set_color(const Color& color) const;
  const Mutator& set_color(Color&& color) const;

  const Mutator& set_pattern(Pattern pattern) const;

  const Mutator& set_period(uint16_t period_ms) const;

  const Mutator& set_transition_period(uint16_t transition_ms) const;
};

void Init();

// Immediately turns off all RGB LEDs.
void Clear();

}  // namespace Rgb