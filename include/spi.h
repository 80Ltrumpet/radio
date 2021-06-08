#pragma once

#include <stdint.h>

#include "atomic.h"

class Spi final {
  using SlaveSelector = void (*)(bool);

 public:
  static constexpr uint8_t kAddrWrite{0x80};
  static constexpr uint8_t kAddrRead{};
  
  class Transaction final : public AtomicLock {
   public:
    explicit Transaction(const Spi& spi) : spi_{spi} { post_lock(); }
    ~Transaction() { pre_unlock(); }

    void read(void* buffer, uint8_t length) const;
    void write(const void* buffer, uint8_t length) const;
    void rw(void* rbuf, const void* wbuf, uint8_t length) const;

    // Single-byte utilities
    void read(uint8_t& byte) const { read(&byte, 1); }
    void write(uint8_t byte) const { write(&byte, 1); }
    void rw(uint8_t& rbyte, uint8_t wbyte) const { rw(&rbyte, &wbyte, 1); }

   protected:
    void post_lock() override { spi_.ss_(true); }
    void pre_unlock() override { spi_.ss_(false); }

   private:
    const Spi& spi_;
  };

  Spi(SlaveSelector ss, void (*ss_configure)() = nullptr) : ss_{ss} { 
    if (ss_configure) ss_configure();
    ss(false);
  }

  // Must be called before calling any Transaction methods.
  static void Init();

 private:
  SlaveSelector ss_;
};