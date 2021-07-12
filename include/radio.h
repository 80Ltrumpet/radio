#pragma once

#include "spi.h"

// Low-level radio communication API. Networking will be handled by a separate
// module (task).
namespace Radio {

constexpr uint8_t kInvalidAddr{0xff};
constexpr uint8_t kBroadcastAddr{RADIO_BROADCAST_ADDR};

struct Packet final {
  const uint8_t length;  // Excludes this byte
  const uint8_t dest;
  const uint8_t src;
  uint8_t payload[0];
};
static_assert(sizeof(Packet) == 3, "Radio::Packet has wrong size.");

// These handlers are called from interrupt context.
class EventHandler final {
  using ReadyCb = void (*)(int8_t);
  using SentCb = void (*)();

 public:
  EventHandler() = default;
  EventHandler(const EventHandler&) = delete;
  EventHandler(ReadyCb payload_ready, SentCb packet_sent)
      : on_payload_ready_{payload_ready}, on_packet_sent_{packet_sent} {}

  inline void on_payload_ready(int8_t rssi) {
    if (on_payload_ready_) on_payload_ready_(rssi);
  }

  inline void on_packet_sent() {
    if (on_packet_sent_) on_packet_sent_();
  }

 private:
  ReadyCb on_payload_ready_{};
  SentCb on_packet_sent_{};
};

void Init();

uint8_t GetNodeAddress();
void SetNodeAddress(uint8_t addr);

void SetEventHandler(EventHandler&& handler);

void Listen(bool use_rx = false);

void HandlePacket(void (*handler)(const Packet&));
bool SendPacket(uint8_t dest, const void* data, uint8_t length);

}  // namespace Radio