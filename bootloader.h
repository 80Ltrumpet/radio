#pragma once

#include <stdint.h>

namespace Bootloader {

constexpr uint16_t kMagicKey{0x7777};
constexpr uint16_t kMagicKeyAddr{0x8000};
constexpr uint16_t kNewLufaSignature{0xdcfb};

}