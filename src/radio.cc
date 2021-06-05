#include "radio.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_registry.h"
#include "eeprom.h"
#include "rfm69hcw.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"

using namespace rfm69hcw;

namespace {

/*------------------------------------------------------------------------------
 * Constants and types
 */
constexpr const uint8_t kSyncWords[]{RADIO_SYNC_WORDS};
constexpr uint8_t kBroadcastAddr{RADIO_BROADCAST_ADDR};

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

/*------------------------------------------------------------------------------
 * Radio stuff
 */

uint8_t node_addr_{0xff};

// Sets recommended defaults according to the data sheet.
void set_defaults() {
  // The longest (and only) burst write is Lna through AfcBw.
  constexpr uint8_t kBufLen{Reg::AfcBw - Reg::Lna + 1};
  const uint8_t buffer[kBufLen]{Default::Lna, Default::RxBw, Default::AfcBw};
  write(Reg::Lna, buffer, kBufLen);
  write(Reg::DioMapping2, Default::DioMapping2);
  write(Reg::RssiThresh, Default::RssiThresh);
  write(Reg::FifoThresh, Default::FifoThresh);
  write(Reg::TestDagc, Default::TestDagc);
}

// Performs one-time configuration.
void configure() {
  // Write the sync words before configuring their length.
  const uint8_t kSyncWordsLength{sizeof(kSyncWords)};
  write(Reg::SyncValue, kSyncWords, kSyncWordsLength);
  // If the length is not equal to the default, configure it.
  if (auto sync_config{Reset::SyncConfig};
      sync_config != ((Reset::SyncConfig & SyncSize) >> SyncSize_)) {
    sync_config &= SyncSize;
    sync_config |= (kSyncWordsLength - 1) << SyncSize_;
    write(Reg::SyncConfig, sync_config);
  }

  // Use packet-mode GFSK (BT=1) at 200 kbps with a 433 MHz carrier frequency.
  constexpr uint8_t kBufLen{Reg::Osc1 - Reg::DataModul};
  uint8_t buffer[kBufLen]{
      DataModePacket | ModulationTypeFsk | ModulationShapingFskBt1p0,
      0x00,  // BitRateMsb
      0xa0,  // BitRateLsb
      0x0c,  // FdevMsb
      0xce,  // FdevLsb
      0x6c,  // FrfMsb
      0x4f,  // FrfMid
      0xf8,  // FrfLsb
  };
  write(Reg::DataModul, buffer, kBufLen);

  // Set the channel filter bandwidth to 400 kHz with a 500 Hz DCC cutoff.
  buffer[0] = buffer[1] = (7 << DccFreq_) | RxBwMant20;
  write(Reg::RxBw, buffer, 2);

  // Use variable-length packets with DC-free whitening and CRC checks. Use
  // node and broadcast address filtering.
  write(Reg::PacketConfig1, PacketFormatVariable | DcFreeWhitening | CrcOn |
                                AddressFilteringBroadcast);

  // Read the node address programmed into the EEPROM.
  Eeprom::Read(Eeprom::Addr::NodeAddress, &node_addr_, sizeof(node_addr_));
  // Set the node and broadcast addresses.
  buffer[0] = node_addr_;
  buffer[1] = kBroadcastAddr;
  write(Reg::NodeAdrs, buffer, 2);

  // Leave everything else at their default value.
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
  // TODO: Eventually, this should move to the network layer. Once a valid node
  // address is assigned, the node can start attempting to participate in the
  // network.
  if (argc < 2 || strcmp(argv[1], "addr") != 0) {
    puts("Usage: radio addr [ADDRESS]");
    return;
  }

  if (argc < 3) {
    printf("%02" PRIx8 "\n", node_addr_);
    return;
  }

  errno = 0;
  auto addr{strtol(argv[2], nullptr, 16)};
  if (errno != 0 || addr >= 0xff) {
    printf("Invalid hexadecimal byte \"%s\".\n", argv[2]);
  }
  Eeprom::Update(Eeprom::Addr::NodeAddress, &addr, 1);
  node_addr_ = addr;
  write(Reg::NodeAdrs, node_addr_);
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

  // Configure the external interrupt (rising edge)
  DDRB &= ~_BV(DDB6);
  PORTB &= ~_BV(PORTB6);
  EICRB = _BV(ISC61) | _BV(ISC60);
  EIMSK |= _BV(INT6);

  set_defaults();
  configure();

  // TODO: At this point, the network layer should manage the state machine.
  // Nodes should start by periodically broadcasting discovery packets until one
  // is acknowledged. The root starts in the listening state.
}

uint8_t GetNodeAddress() { return node_addr_; }

}  // namespace Radio

/*------------------------------------------------------------------------------
 * Interrupt handler
 */
ISR(INT6_vect) {
  // TODO
}