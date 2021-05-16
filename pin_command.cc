#include "pin_command.h"

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"

namespace {

// There are 54 pins labeled "DIGITAL" on the Arduino MEGA 2560 silk screen.
constexpr uint8_t kPinCount{54};

// Each of these values is an encoded port-bit pair, with a nibble for each.
const uint8_t kPin[kPinCount]{
    0x40, 0x41, 0x44, 0x45, 0x65, 0x43, 0x73, 0x74, 0x75, 0x76, 0x14,
    0x15, 0x16, 0x17, 0x81, 0x80, 0x71, 0x70, 0x33, 0x32, 0x31, 0x30,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x27, 0x26, 0x25,
    0x24, 0x23, 0x22, 0x21, 0x20, 0x37, 0x62, 0x61, 0x60, 0xa7, 0xa6,
    0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0, 0x13, 0x12, 0x11, 0x10};

}  // namespace

void PinCommand::CommandHandler([[maybe_unused]] int argc,
                                [[maybe_unused]] const char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return;
  }
  DDRA &= ~_BV(DDA0);

  if (strcmp("dir", argv[1]) == 0) {
    // TODO: This subcommand doesn't work. :/
    if (argc < 4) {
      PrintUsage();
      return;
    }
    auto pin{atoi(argv[2])};
    if (pin < 0 || pin >= kPinCount) {
      printf("Pin %d does not exist.\n", pin);
      return;
    }
    bool out;
    if (strcmp("in", argv[3]) == 0) {
      out = false;
    } else if (strcmp("out", argv[3]) == 0) {
      out = true;
    } else {
      printf("Invalid direction \"%s\".\n", argv[3]);
      return;
    }
    // Convert the port to the DDRx address.
    uint16_t addr{kPin[pin] >> 4};
    if (addr < 7) {
      addr = addr * 3 + _SFR_ADDR(DDRA);
    } else {
      addr = (addr - 7) * 3 + _SFR_ADDR(DDRH);
    }
    uint8_t bit{_BV(kPin[pin] & 0xf)};
    printf("%04" PRIx16 ": %02" PRIx8 "\n", addr, _SFR_MEM8(addr));
    _SFR_MEM8(addr) = out ? _SFR_MEM8(addr) | bit : _SFR_MEM8(addr) & ~bit;
    _NOP();
    printf("%04" PRIx16 ": %02" PRIx8 "\n", addr, _SFR_MEM8(addr));
  } else if (strcmp("set", argv[1]) == 0) {
    if (argc < 4) {
      PrintUsage();
      return;
    }
    auto pin{atoi(argv[2])};
    if (pin < 0 || pin >= kPinCount) {
      printf("Pin %d does not exist.\n", pin);
      return;
    }
    bool set;
    if (*argv[3] == '0' && argv[3][1] == '\0') {
      set = false;
    } else if (*argv[3] == '1' && argv[3][1] == '\0') {
      set = true;
    } else {
      printf("Invalid value \"%s\".\n", argv[3]);
      return;
    }
    // Convert the port to the PORTx address.
    uint16_t addr{kPin[pin] >> 4};
    if (addr < 7) {
      addr = addr * 3 + _SFR_ADDR(PORTA);
    } else {
      addr = (addr - 7) * 3 + _SFR_ADDR(PORTH);
    }
    uint8_t bit{_BV(kPin[pin] & 0xf)};
    printf("%04" PRIx16 ": %02" PRIx8 "\n", addr, _SFR_MEM8(addr));
    _SFR_MEM8(addr) = set ? _SFR_MEM8(addr) | bit : _SFR_MEM8(addr) & ~bit;
    _NOP();
    printf("%04" PRIx16 ": %02" PRIx8 "\n", addr, _SFR_MEM8(addr));
  } else if (strcmp("dump", argv[1]) == 0) {
    if (argc < 3) {
      // Read all of the relevant registers into a flat array.
      uint8_t data[33];
      uint8_t i{};
      for (auto addr{_SFR_ADDR(PINA)}; addr < _SFR_ADDR(TIFR0); ++addr) {
        data[i++] = _SFR_MEM8(addr);
      }
      for (auto addr{_SFR_ADDR(PINH)}; addr < 0x10c; ++addr) {
        data[i++] = _SFR_MEM8(addr);
      }
      puts("ID PIN/DDR/PORT");
      for (uint8_t j{}; j < i; ++j) {
        uint8_t pinx{(kPin[j] >> 4) * 3};
        uint8_t mask{_BV(kPin[j] & 0xf)};
        printf("%2" PRIu8 " %" PRIu8 "/%" PRIu8 "/%" PRIu8 "\n", j,
               !!(data[pinx] & mask), !!(data[pinx + 1] & mask),
               !!(data[pinx + 2] & mask));
      }
    } else {
      auto pin{atoi(argv[2])};
      if (pin < 0 || pin >= kPinCount) {
        printf("Pin %d does not exist.\n", pin);
        return;
      }
      uint16_t addr{kPin[pin] >> 4};
      if (addr < 7) {
        addr = addr * 3 + _SFR_ADDR(PINA);
      } else {
        addr = (addr - 7) * 3 + _SFR_ADDR(PINH);
      }
      uint8_t mask{_BV(kPin[pin] & 0xf)};
      puts("ID PIN/DDR/PORT");
      printf("%2" PRIu8 " %" PRIu8 "/%" PRIu8 "/%" PRIu8 "\n", pin,
             !!(_SFR_MEM8(addr) & mask), !!(_SFR_MEM8(addr + 1) & mask),
             !!(_SFR_MEM8(addr + 2) & mask));
    }
  } else {
    PrintUsage();
  }
}

void PinCommand::PrintUsage() {
  puts(
      "Usage: pin dump [<id>]\n"
      "           dir <id> in|out\n"
      "           set <id> 1|0");
}

const char* const PinCommand::kCommandName{"pin"};

const bool PinCommand::registered{
    CommandRegistry::RegisterCommand<PinCommand>()};