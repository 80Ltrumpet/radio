#pragma once

#include <stdint.h>
#include <stdio.h>

// TODO: Create interface for console.

class Usb final {
 public:
  static void Init();

  // Provide a more efficient way to get a character than getc().
  static bool GetChar(char& c);

 private:
  Usb() = delete;

  // Hook for stdio output streams.
  static int PutChar(char c, FILE* stream);
};