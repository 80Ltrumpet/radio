#pragma once

#include "console.h"

class Timer final : public Console::Command<Timer> {
public:
  // This class is purely static, so don't allow instantiation.
  Timer() = delete;

  static void Init();

  static inline volatile unsigned long long GetTicks();

private:
  // Executes the command with the given arguments.
  static void Execute(int argc, const char* argv[]);

  static const char* const kCommandName;
};