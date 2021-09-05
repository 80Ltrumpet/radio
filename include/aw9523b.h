#pragma once

#include <stdint.h>

namespace aw9523b {

constexpr uint8_t kI2cAddr{0x58};
constexpr uint8_t kId{0x23};

// Register address definitions from Table 4 of the data sheet
namespace Reg {

constexpr uint8_t INPUT0{0x00};
constexpr uint8_t INPUT1{0x01};
constexpr uint8_t OUTPUT0{0x02};
constexpr uint8_t OUTPUT1{0x03};
constexpr uint8_t CONFIG0{0x04};
constexpr uint8_t CONFIG1{0x05};
constexpr uint8_t INT_DISABLE0{0x06};
constexpr uint8_t INT_DISABLE1{0x07};
constexpr uint8_t ID{0x10};
constexpr uint8_t CTL{0x11};
constexpr uint8_t MODE0{0x12};
constexpr uint8_t MODE1{0x13};
constexpr uint8_t DIM(uint8_t n) { return 0x20 + (n & 0xf); }
constexpr uint8_t SW_RSTN{0x7f};

namespace Reset {

constexpr uint8_t ID{kId};
constexpr uint8_t MODE0{0xff};
constexpr uint8_t MODE1{0xff};

}  // namespace Reset

}  // namespace Reg

namespace Bits {

// CTL
constexpr uint8_t GPOMD{0x10};
constexpr uint8_t ISEL{0x03};
enum ISEL_value : uint8_t {
  ISEL_37MA,
  ISEL_28MA,
  ISEL_19MA,
  ISEL_9MA,
};

}  // namespace Bits

// Converts a port mask to a word-register mask.
constexpr uint16_t WordRegMask(uint16_t port_mask) {
  return (port_mask & 0xf000) | ((port_mask & 0x000f) << 8) |
         ((port_mask & 0x0ff0) >> 4);
}

}  // namespace aw9523b