#pragma once

#include <stdint.h>

namespace Eeprom {

// This is a table of well-known data locations within the EEPROM address space.
// The length of the data stored at each address is specified in a comment to
// aid provisioning (i.e., avoid overlapping new entries over old allocations).
struct Addr final {
#define ADDR_T static constexpr uint16_t
  ADDR_T NodeAddress{0x0000};  // 1
#undef ADDR_T
};

void Erase(uint16_t addr, uint16_t length);
uint16_t Read(uint16_t addr, void* buffer, uint16_t length);
void Update(uint16_t addr, const void* buffer, uint16_t length);

}  // namespace Eeprom