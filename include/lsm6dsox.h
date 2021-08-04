#pragma once

#include <stdint.h>

// TODO: This isn't a great interface...
namespace lsm6dsox {

struct FifoData final {
  union Tag {
    struct {
      const uint8_t parity : 1;
      const uint8_t counter : 2;
      const uint8_t sensor : 5;
    };
    const uint8_t value;
  };

  union Sample {
    int16_t words[3];
    struct {
      int16_t x;
      int16_t y;
      int16_t z;
    };
  };

  const Sample& get_sample() const {
    return *reinterpret_cast<const Sample*>(bytes);
  }

  Tag tag;
  uint8_t bytes[sizeof(Sample)];
};
static_assert(sizeof(FifoData) == 7, "lsm6dsox::FifoData has the wrong size");

bool Exists();
void Configure();
void Enable();
void Disable();
void ProcessFifoData(void (*)(const FifoData&));

}  // namespace lsm6dsox