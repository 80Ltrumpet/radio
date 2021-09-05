#pragma once

#include "rgb.h"

namespace Puzzle {

struct Header {
  enum Type : uint8_t {
    kTypePutDown,
    kTypePickUp,
    kTypeGetState,
    kTypeLedControl,
  };

  Header(Type t) : type{t} {}

  const Type type;
};
static_assert(sizeof(Header) == 1, "Puzzle::Header size is incorrect");

class LedControlPacket final : public Header {
  using Color = Rgb::Color;
  using Pattern = Rgb::Pattern;

 public:
  LedControlPacket() : Header{kTypeLedControl} {}

  const Color* get_color() const {
    return flags_ & kFlagColor ? &color_ : nullptr;
  }

  const Pattern* get_pattern() const {
    return flags_ & kFlagPattern ? &pattern_ : nullptr;
  }

  const uint16_t* get_period() const {
    return flags_ & kFlagPeriod ? &period_ms_ : nullptr;
  }

  const uint16_t* get_transition_period() const {
    return flags_ & kFlagTransitionPeriod ? &transition_ms_ : nullptr;
  }

  void set_color(const Color& color) {
    flags_ |= kFlagColor;
    color_ = color;
  }

  void set_color(Color&& color) {
    flags_ |= kFlagColor;
    color_ = static_cast<Color&&>(color);
  }

  void set_pattern(Pattern pattern) {
    flags_ |= kFlagPattern;
    pattern_ = pattern;
  }

  void set_period(uint16_t period_ms) {
    flags_ |= kFlagPeriod;
    period_ms_ = period_ms;
  }

  void set_transition_period(uint16_t transition_ms) {
    flags_ |= kFlagTransitionPeriod;
    transition_ms_ = transition_ms;
  }

 private:
  enum Flag : uint8_t {
    kFlagColor = 0x01,
    kFlagPattern = 0x02,
    kFlagPeriod = 0x04,
    kFlagTransitionPeriod = 0x08,
  };

  uint8_t flags_{};
  Color color_{};
  Pattern pattern_{Pattern::None};
  uint16_t period_ms_{};
  uint16_t transition_ms_{};
};
static_assert(sizeof(LedControlPacket) == 10,
              "Puzzle::LedControlPacket size is incorrect");

void Init();

}  // namespace Puzzle