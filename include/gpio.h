#pragma once

#include <stdint.h>

class Gpio final {
 public:
  explicit Gpio(volatile uint8_t& pinx, uint8_t bit_number)
      : pinx_{&pinx}, mask_{1 << bit_number} {}

  inline uint8_t get() const { return pinx() & mask_; }

  inline void in() const { ddrx() &= ~mask_; }
  inline void out() const { ddrx() |= mask_; }

  inline void set() const { portx() |= mask_; }
  inline void clear() const { portx() &= ~mask_; }
  inline void toggle() const { portx() ^= mask_; }

 private:
  inline constexpr volatile uint8_t& pinx() const { return *pinx_; }
  inline constexpr volatile uint8_t& ddrx() const { return *(pinx_ + 1); }
  inline constexpr volatile uint8_t& portx() const { return *(pinx_ + 2); }

  volatile uint8_t* pinx_;
  const uint8_t mask_;
};