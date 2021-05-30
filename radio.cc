#include "radio.h"

#include <avr/io.h>
#include <stdio.h>

#include "command_registry.h"
#include "rfm69hcw.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"

using namespace rfm69hcw;

namespace {

/*------------------------------------------------------------------------------
 * SPI
 */
void slave_select(bool select) {
  // SS is asserted low and has an external pull-up.
  if (select) {
    // Current drain
    DDRB |= _BV(DDB4);
  } else {
    // External pull-up
    DDRB &= ~_BV(DDB4);
  }
}

const Spi spi_{slave_select, []() { PORTB &= ~_BV(PORTB4); }};

void read(uint8_t addr, void* buffer, uint8_t length) {
  Spi::Transaction spi{spi_};
  spi.write(addr | Spi::kAddrRead);
  spi.read(buffer, length);
}

inline uint8_t read(uint8_t addr) {
  uint8_t byte;
  read(addr, &byte, 1);
  return byte;
}

void write(uint8_t addr, const void* buffer, uint8_t length) {
  Spi::Transaction spi{spi_};
  spi.write(addr | Spi::kAddrWrite);
  spi.write(buffer, length);
}

inline void write(uint8_t addr, uint8_t byte) { write(addr, &byte, 1); }

void init_defaults() {
  // The longest (and only) burst write is Lna through AfcBw.
  constexpr uint8_t kBufLen{Reg::AfcBw - Reg::Lna + 1};
  uint8_t buffer[kBufLen]{Default::Lna, Default::RxBw, Default::AfcBw};
  write(Reg::Lna, buffer, kBufLen);
  write(Reg::DioMapping2, Default::DioMapping2);
  write(Reg::RssiThresh, Default::RssiThresh);
  write(Reg::FifoThresh, Default::FifoThresh);
  write(Reg::TestDagc, Default::TestDagc);
}

void run([[maybe_unused]] void* arg) {
  // TODO: State machine, etc.
}

}  // namespace

/*------------------------------------------------------------------------------
 * Command
 */
class RadioCommand final {
 public:
  static void CommandHandler(int argc, const char* argv[]);

  static const char* const kCommandName;

 private:
  static const bool registered;
};

void RadioCommand::CommandHandler([[maybe_unused]] int argc,
                                  [[maybe_unused]] const char* argv[]) {
  // For now, just read the version register.
  printf("%02" PRIx8 "\n", read(Reg::Version));
}

const char* const RadioCommand::kCommandName{"radio"};
const bool RadioCommand::registered{
    CommandRegistry::RegisterCommand<RadioCommand>()};

/*------------------------------------------------------------------------------
 * Public API
 */
namespace Radio {

void Init() {
  // Make sure the reset line is floating.
  DDRD &= ~_BV(DDD4);
  PORTD &= ~_BV(PORTD4);

  // Wait until the reset line is low.
  while (PIND & _BV(PIND4))
    ;
  // Wait at least 10 milliseconds to ensure the chip is out of reset.
  Timer::DelayMs(10);

  // Add the RFM69HCW task to the scheduler.
  Scheduler::AddTask({"radio", run});
}

}  // namespace Radio