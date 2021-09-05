#pragma once

#include "spi.h"

// Low-level radio communication API. Networking will be handled by a separate
// module (task).
namespace Radio {

constexpr uint8_t kInvalidAddr{0xff};
constexpr uint8_t kBroadcastAddr{RADIO_BROADCAST_ADDR};

constexpr int8_t kRssiInvalid{0x7f};

struct IFifoBuffer {
  static constexpr uint8_t kSize{66};
  virtual const void* cget() const = 0;
  virtual void* get() = 0;
  virtual const uint8_t* cbytes() const = 0;
  virtual uint8_t* bytes() = 0;

  constexpr uint8_t size() const { return kSize; }
};

// FIFO buffer wrapper with internally allocated memory
class FifoBuffer final : public IFifoBuffer {
 public:
  const void* cget() const override {
    return reinterpret_cast<const void*>(buffer_);
  }
  void* get() override { return reinterpret_cast<void*>(buffer_); }
  const uint8_t* cbytes() const override { return buffer_; }
  uint8_t* bytes() override { return buffer_; }

 private:
  uint8_t buffer_[kSize];
};

// FIFO buffer wrapper that assumes the pointer is allocated with kSize bytes
class FifoBufferPtr final : public IFifoBuffer {
 public:
  explicit constexpr FifoBufferPtr(void* ptr) : ptr_{ptr} {}
  const void* cget() const override { return ptr_; }
  void* get() override { return ptr_; }
  const uint8_t* cbytes() const override {
    return reinterpret_cast<const uint8_t*>(ptr_);
  }
  uint8_t* bytes() override { return reinterpret_cast<uint8_t*>(ptr_); }

 private:
  void* ptr_;
};

struct Header final {
  const uint8_t length;  // Excludes this byte
  const uint8_t dest;
  const uint8_t src;
};
static_assert(sizeof(Header) == 3, "Radio::Header has wrong size.");

struct Client final {
  void (*on_payload_ready)(int8_t /* rssi */){};
  void (*on_packet_sent)(){};
};

void Init();

uint8_t GetAddress();
void SetAddress(uint8_t addr);

void SetClient(Client&& client);

void Listen(bool high_power = false);

bool Receive(IFifoBuffer* buffer);
bool Send(uint8_t dest, const void* data, uint8_t length);

}  // namespace Radio