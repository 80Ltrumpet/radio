#pragma once

#include <stdint.h>

// TODO: This isn't a great interface...
namespace lis3mdl {

struct Sample final {
  int16_t x;
  int16_t y;
  int16_t z;
};
static_assert(sizeof(Sample) == 6, "lis3mdl::Sample has incorrect size");

bool Exists();
void Configure();
void Enable();
void Disable();
bool Available();
Sample Poll();

}