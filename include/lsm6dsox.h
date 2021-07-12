#pragma once

#include <stdint.h>

// TODO: This isn't a great interface...
namespace lsm6dsox {

struct FifoDatum final {
  int16_t x() const { return (static_cast<int16_t>(xh) << 8) | xl; }
  int16_t y() const { return (static_cast<int16_t>(yh) << 8) | yl; }
  int16_t z() const { return (static_cast<int16_t>(zh) << 8) | zl; }

  union {
    struct {
      uint8_t parity : 1;
      uint8_t counter : 2;
      uint8_t sensor : 5;
    };
    uint8_t tag;
  };
  uint8_t xl;
  uint8_t xh;
  uint8_t yl;
  uint8_t yh;
  uint8_t zl;
  uint8_t zh;
};
static_assert(sizeof(FifoDatum) == 7, "FifoDatum has the wrong size");

bool Exists();
void Configure();
void Enable();
void Disable();
void ProcessFifoData(void (*)(const FifoDatum&));

}  // namespace lsm6dsox