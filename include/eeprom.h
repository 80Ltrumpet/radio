#pragma once

#include <stdint.h>

namespace Eeprom {

void Erase(uint16_t addr, uint16_t length);
uint16_t Read(uint16_t addr, void* buffer, uint16_t length);
void Update(uint16_t addr, const void* buffer, uint16_t length);

namespace Data {

struct Data_ final {
  constexpr Data_(uint16_t addr, uint16_t len) : addr{addr}, length{len} {}
  const uint16_t addr;
  const uint16_t length;
};

// This is a table of well-known data locations within the EEPROM address space.
constexpr Data_ RadioAddress{0x0000, 1};

inline void Erase(const Data_& data) {
  ::Eeprom::Erase(data.addr, data.length);
}

inline uint16_t Read(const Data_& data, void* buffer) {
  return ::Eeprom::Read(data.addr, buffer, data.length);
}

inline void Update(const Data_& data, const void* buffer) {
  ::Eeprom::Update(data.addr, buffer, data.length);
}

}  // namespace Data

}  // namespace Eeprom