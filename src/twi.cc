#include "twi.h"

#include <alloca.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"

// Status codes
enum Tws : uint8_t {
  kTwsBusError = 0x00,
  kTwsStart = 0x08,
  kTwsRepeatedStart = 0x10,
  kTwsSlawAck = 0x18,
  kTwsSlawNack = 0x20,
  kTwsWriteAck = 0x28,
  kTwsWriteNack = 0x30,
  kTwsArbitrationLost = 0x38,
  kTwsSlarAck = 0x40,
  kTwsSlarNack = 0x48,
  kTwsReadAck = 0x50,
  kTwsReadNack = 0x58,
  // We never operate in Slave Receiver or Slave Transmitter mode, so the status
  // codes specific to those are not included.
  kTwsMask = 0xf8,
};

enum Sla : uint8_t {
  kSlaWrite,
  kSlaRead,
};

namespace {

struct {
  size_t length{};
  uint8_t* buffer{};
  uint8_t addr{};
  uint8_t sla{};
  Sla type{};
} transaction_{};

void wait_for_twint() {
  while (!(TWCR & _BV(TWINT)))
    ;
}

inline void start() { TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA); }

inline void stop() {
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
  while (TWCR & _BV(TWSTO))
    ;
}

inline void write_byte(uint8_t byte) {
  TWDR = byte;
  TWCR = _BV(TWINT) | _BV(TWEN);
}

inline void read_byte(bool ack) {
  TWCR = _BV(TWINT) | _BV(TWEN) | (ack ? _BV(TWEA) : 0);
}

bool twi_state_machine() {
  wait_for_twint();

  switch (static_cast<Tws>(TWSR & kTwsMask)) {
    case kTwsStart:
      // Set up sub-address write.
      write_byte((transaction_.sla << 1) | kSlaWrite);
      break;
    case kTwsRepeatedStart:
      // Perform the read portion of the transaction.
      write_byte((transaction_.sla << 1) | kSlaRead);
      break;
    case kTwsSlawAck:
      // Send sub-address.
      write_byte(transaction_.addr);
      break;
    case kTwsWriteAck:
      // If this is a write transaction, send data from the buffer. Otherwise,
      // perform a repeated start to switch to Master Receiver mode.
      if (transaction_.type == kSlaWrite) {
        // Check if the write transaction is complete.
        if (transaction_.length == 0) {
          stop();
          return false;
        }
        --transaction_.length;
        write_byte(*transaction_.buffer++);
      } else {
        start();
      }
      break;
    case kTwsReadAck:
      // Put the byte into the buffer.
      --transaction_.length;
      *transaction_.buffer++ = TWDR;
      [[fallthrough]];
    case kTwsSlarAck:
      read_byte(transaction_.length > 1);
      break;
    case kTwsReadNack:
      // Put the final byte into the buffer.
      *transaction_.buffer = TWDR;
      [[fallthrough]];
    case kTwsSlawNack:
    case kTwsSlarNack:
    case kTwsWriteNack:
      // Stop the transaction.
      stop();
      return false;
    case kTwsArbitrationLost:
      // Unlikely error.
      read_byte(true);
      return false;
  }

  return true;
}

void execute_transaction() {
  // Kick off the state machine and let it run its course.
  start();
  while (twi_state_machine())
    ;
}

}  // namespace

void Twi::Device::read(uint8_t addr, void* buffer, size_t length) const {
  transaction_.buffer = reinterpret_cast<uint8_t*>(buffer);
  transaction_.length = length;
  transaction_.addr = addr;
  transaction_.sla = sla_;
  transaction_.type = kSlaRead;
  execute_transaction();
}

void Twi::Device::write(uint8_t addr, const void* buffer,
                        size_t length) const {
  transaction_.buffer = reinterpret_cast<uint8_t*>(const_cast<void*>(buffer));
  transaction_.length = length;
  transaction_.addr = addr;
  transaction_.sla = sla_;
  transaction_.type = kSlaWrite;
  execute_transaction();
}

void Twi::Init() {
#ifdef ARDUINO_AVR_FEATHER32U4
  // According to the 32u4 data sheet, TWBR should not be less than 10 when
  // operating in master mode to avoid a hardware limitation. Unfortunately,
  // this means the highest achievable bit rate with an 8 MHz clock is ~222 kHz.
  // With a TWBR of 12, we get a nice, round 200 kHz.
  TWBR = 12;
#else
  // Set bit rate as close to 400 kHz as possible.
  TWBR = ((F_CPU / 4000000) - 16) >> 1;
#endif
}

struct TwiCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

  static void Read(int argc, const char* argv[]);
  static void Write(int argc, const char* argv[]);
  static void PrintUsage();

 private:
  struct Addresses {
    uint8_t dev;
    uint8_t sub;
    bool valid{false};
  };

  static Addresses GetAddresses(const char* argv[]);

  static const bool registered;
};

void TwiCommand::CommandHandler(int argc, const char* argv[]) {
  if (argc < 5) {
    PrintUsage();
    return;
  }

  if (strcmp(argv[1], "read") == 0) {
    Read(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "write") == 0) {
    Write(argc - 2, argv + 2);
  } else {
    PrintUsage();
  }
}

// Common input parsing logic
TwiCommand::Addresses TwiCommand::GetAddresses(const char* argv[]) {
  Addresses addr;
  char* endptr{};
  auto dev_addr{strtol(argv[0], &endptr, 16)};
  if (*endptr != '\0' || dev_addr < 0 || dev_addr > 0x7f) {
    printf("Invalid device address \"%s\".\n", argv[0]);
    return addr;
  }
  addr.dev = dev_addr;

  endptr = nullptr;
  auto sub_addr{strtol(argv[1], &endptr, 16)};
  if (*endptr != '\0' || sub_addr < 0 || sub_addr > 0xff) {
    printf("Invalid sub address \"%s\".\n", argv[1]);
    return addr;
  }
  addr.sub = sub_addr;
  addr.valid = true;

  return addr;
}

void TwiCommand::Read(int argc, const char* argv[]) {
  if (argc < 3) {
    PrintUsage();
    return;
  }

  auto addr{GetAddresses(argv)};
  if (!addr.valid) return;

  char* endptr{};
  auto length{strtol(argv[2], &endptr, 10)};
  if (*endptr != '\0' || length <= 0 || length > 0xff) {
    printf("Invalid length \"%s\".\n", argv[2]);
    return;
  }

  auto buffer{reinterpret_cast<uint8_t*>(alloca(length))};
  Twi::Device dev{addr.dev};
  dev.read(addr.sub, buffer, length);
  long i{};
  for (; i < length; ++i) {
    printf(" %02" PRIx8, buffer[i]);
    if ((i & 0xf) == 0xf) puts("");
  }
  if ((i & 0xf) != 0xf) puts("");
}

void TwiCommand::Write(int argc, const char* argv[]) {
  if (argc < 3) {
    PrintUsage();
    return;
  }

  auto addr{GetAddresses(argv)};
  if (!addr.valid) return;

  argc -= 2;
  argv += 2;
  auto buffer{reinterpret_cast<uint8_t*>(alloca(argc))};
  for (int i{}; i < argc; ++i) {
    char* endptr{};
    auto& byte{buffer[i]};
    byte = strtol(argv[i], &endptr, 16);
    if (*endptr != '\0' || byte < 0 || byte > 0xff) {
      printf("Invalid byte (%d) \"%s\".\n", i, argv[i]);
      return;
    }
  }

  Twi::Device dev{addr.dev};
  dev.write(addr.sub, buffer, argc);
}

void TwiCommand::PrintUsage() {
  puts(
      "Usage: twi read  DEVICE ADDR LENGTH\n"
      "       twi write DEVICE ADDR BYTE [...]");
}

const char* const TwiCommand::kCommandName{"twi"};
const bool TwiCommand::registered{
    CommandRegistry::RegisterCommand<TwiCommand>()};
