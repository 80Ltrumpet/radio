#pragma once

#include <stddef.h>
#include <stdint.h>

struct Twi final {
  class Device final {
   public:
    explicit constexpr Device(uint8_t sla) : sla_{sla} {}

    void read(uint8_t addr, void* buffer, size_t length) const;
    void write(uint8_t addr, const void* buffer, size_t length) const;

    // Single-byte utilities
    uint8_t read(uint8_t addr) const {
      uint8_t byte;
      read(addr, &byte, 1);
      return byte;
    }
    inline void write(uint8_t addr, uint8_t byte) const {
      write(addr, &byte, 1);
    }

   private:
    const uint8_t sla_;
  };

  static void Init();
};