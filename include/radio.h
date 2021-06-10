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

// These handlers are called from interrupt context.
class EventHandler final {
  using Callback = void (*)();

 public:
  EventHandler() = default;
  EventHandler(const EventHandler&) = delete;
  EventHandler(Callback payload_ready, Callback packet_sent)
      : on_payload_ready_{payload_ready}, on_packet_sent_{packet_sent} {}

  inline void on_payload_ready() {
    if (on_payload_ready_) on_payload_ready_();
  }

  inline void on_packet_sent() {
    if (on_packet_sent_) on_packet_sent_();
  }

 private:
  Callback on_payload_ready_{};
  Callback on_packet_sent_{};
};

void Init();

uint8_t GetNodeAddress();
void SetNodeAddress(uint8_t addr);

void SetEventHandler(EventHandler&& handler);

void Listen();

void HandlePacket(void (*handler)(const Packet&));
void SendPacket(const Packet& packet);

}  // namespace Radio