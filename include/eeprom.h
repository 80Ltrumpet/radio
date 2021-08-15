#pragma once

#include <stdint.h>

namespace Eeprom {

struct Datum final {
  constexpr Datum(uint16_t addr, uint16_t len) : address{addr}, length{len} {}
  const uint16_t address;
  const uint16_t length;
};

// This is a table of well-known data locations within the EEPROM address space.
namespace Data {

constexpr Datum RadioAddress{0x0000, 1};

}  // namespace Data

void Erase(const Datum& datum);
uint16_t Read(const Datum& datum, void* output);
void Update(const Datum& datum, const void* input);

}  // namespace Eeprom