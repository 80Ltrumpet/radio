#pragma once

struct PinCommand final {
  static void CommandHandler(int argc, const char* argv[]);

  static const char* const kCommandName;

 private:
  static void PrintUsage();

  static const bool registered;
};