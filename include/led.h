#pragma once

#include "gpio.h"

class Led final {
 public:
  static void Init();
  static inline void On() { gpio_.set(); }
  static inline void Off() { gpio_.clear(); }
  static inline void Toggle() { gpio_.toggle(); }

 private:
  static Gpio gpio_;
};