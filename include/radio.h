#pragma once

#include "spi.h"

// Low-level radio communication API. Networking will be handled by a separate
// module (task).
namespace Radio {

struct Packet final {
  uint8_t length;  // Includes this byte and the address byte.
  uint8_t address;
  uint8_t payload[0];
};

void Init();

// These handlers are called from interrupt context, so they should delegate
// further processing to a task.
void SetOnPayloadReady(void (*on_payload_ready)());
void SetOnPacketSent(void (*on_packet_sent)());
// TODO: Probably also need a timeout handler?

uint8_t GetNodeAddress();

void Listen();

void HandlePacket(void (*handler)(const Packet&));
void SendPacket(const Packet& packet);

}  // namespace Radio