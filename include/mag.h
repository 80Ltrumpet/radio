#pragma once

#include <stdint.h>

namespace Mag {

struct RawSample final {
  int16_t x;
  int16_t y;
  int16_t z;
};
static_assert(sizeof(RawSample) == 6, "Mag::RawSample has incorrect size");

struct Sample final {
  Sample() = default;
  explicit Sample(const RawSample& raw);

  float x{0.0f};
  float y{0.0f};
  float z{0.0f};
};

void Init();

}