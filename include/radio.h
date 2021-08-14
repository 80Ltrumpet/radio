#pragma once

#include "spi.h"

// Low-level radio communication API. Networking will be handled by a separate
// module (task).
namespace Radio {

constexpr uint8_t kInvalidAddr{0xff};
constexpr uint8_t kBroadcastAddr{RADIO_BROADCAST_ADDR};

constexpr int8_t kRssiInvalid{0x7f};

struct Header final {
  const uint8_t length;  // Excludes this byte
  const uint8_t dest;
  const uint8_t src;
};
static_assert(sizeof(Header) == 3, "Radio::Header has wrong size.");

// This handler is called from interrupt context.
using PayloadReadyCallback = void (*)(int8_t);

void Init();

uint8_t GetAddress();
void SetAddress(uint8_t addr);

void SetPayloadReadyCallback(PayloadReadyCallback cb);

void Listen(bool high_power = false);

void HandlePacket(void (*handler)(const void*, const Header&));
bool SendPacket(uint8_t dest, const void* data, uint8_t length);

}  // namespace Radio