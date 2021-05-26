#pragma once

#include <stdint.h>

class Timer final {
 public:
  static void Init();

  static uint64_t Millis();
  static uint64_t Micros();

  static void DelayMs(uint64_t ms);
  static void DelayUs(uint32_t us);

 private:
  Timer() = delete;
};