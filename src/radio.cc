#include "radio.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atomic.h"
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
Radio::EventHandler event_handler_{};

// Shadow register to optimize read-modify-write operations
uint8_t op_mode_{Reg::Reset::OpMode};

// Sets recommended defaults according to the data sheet.
void set_defaults() {
  // The longest (and only) burst write is Lna through AfcBw.
  constexpr uint8_t kBufLen{Reg::AfcBw - Reg::Lna + 1};
  const uint8_t buffer[kBufLen]{Reg::Default::Lna, Reg::Default::RxBw,
                                Reg::Default::AfcBw};
  write(Reg::Lna, buffer, kBufLen);
  write(Reg::DioMapping2, Reg::Default::DioMapping2);
  write(Reg::RssiThresh, Reg::Default::RssiThresh);
  write(Reg::FifoThresh, Reg::Default::FifoThresh);
  write(Reg::TestDagc, Reg::Default::TestDagc);
}

// Performs one-time configuration.
void configure() {
  using namespace Bits;

  // Write the sync words before configuring their length.
  const uint8_t kSyncWordsLength{sizeof(kSyncWords)};
  write(Reg::SyncValue, kSyncWords, kSyncWordsLength);
  // If the length is not equal to the default, configure it.
  if (auto sync_config{Reg::Reset::SyncConfig};
      (sync_config & SyncSize) != (Reg::Reset::SyncConfig & SyncSize)) {
    sync_config &= ~SyncSize;
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

  // Configure listen timings to an 80% duty cycle with ~80 milliseconds of Rx
  // and ~20 milliseconds of idle. Packet acceptance requires an address match.
  // After listening, return to the currently programmed mode.
  buffer[0] = ListenResolIdle4100us | ListenResolRx4100us | ListenCriteria |
              ListenEndMode;
  buffer[1] = 5;   // 20.5 ms
  buffer[2] = 20;  // 82.0 ms
  write(Reg::Listen1, buffer, 3);

  // Leave everything else at default values.
}

void set_listen(bool enable) {
  auto op_mode{op_mode_};
  if ((op_mode & Bits::ListenOn) == enable) return;

  if (enable) {
    op_mode |= Bits::ListenOn;
  } else {
    // Need to set and clear ListenAbort in two separate SPI transactions.
    op_mode = (op_mode | Bits::ListenAbort) & ~Bits::ListenOn;
    write(Reg::OpMode, op_mode);
    op_mode &= ~Bits::ListenAbort;
  }

  write(Reg::OpMode, op_mode);
  op_mode_ = op_mode;
}

void set_op_mode(uint8_t mode) {
  set_listen(false);
  if (auto op_mode{op_mode_}; (op_mode & Bits::Mode) != mode) {
    op_mode = (op_mode & ~Bits::Mode) | mode;
    write(Reg::OpMode, op_mode);
    op_mode_ = op_mode;
  }
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
  Radio::SetNodeAddress(addr);
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

void SetEventHandler(EventHandler&& handler) {
  event_handler_ = static_cast<EventHandler&&>(handler);
}

uint8_t GetNodeAddress() { return node_addr_; }

void SetNodeAddress(uint8_t addr) {
  Eeprom::Update(Eeprom::Addr::NodeAddress, &addr, 1);
  node_addr_ = addr;
  write(Reg::NodeAdrs, node_addr_);
}

void Listen() {
  // Minor optimization (set_listen also checks this).
  if (op_mode_ & Bits::ListenOn) return;

  // Listen mode can only be enabled while in standby (idle).
  set_op_mode(Bits::ModeStdby);

  // Switch the interrupt source to PayloadReady.
  write(Reg::DioMapping1, 1 << Bits::Dio0Mapping_);

  set_listen(true);
}

void HandlePacket(void (*handler)(const Packet&)) {
  // NOTE: Packets greater than the length of the FIFO are not allowed.
  static uint8_t buffer[kFifoSize]{};

  // Check if we even have a packet.
  if (!(read(Reg::IrqFlags2) & Bits::PayloadReady)) return;

  {
    // Small optimization to perform the entire read in a single transaction.
    Spi::Transaction spi{spi_};
    spi.write(Reg::Fifo | Spi::kAddrRead);
    // Read the packet length.
    spi.read(buffer, 1);
    // Read the rest of the packet.
    spi.read(buffer + 1, buffer[0] - 1);
  }

  // Handle the packet.
  handler(*reinterpret_cast<const Packet*>(buffer));
}

void SendPacket(const Packet& packet) {
  // NOTE: This assumes that only one task is responsible for sending packets.
  // Otherwise, we would have to ensure that we are not in transmit mode, first.
  set_op_mode(Bits::ModeStdby);

  // Switch the interrupt source to PacketSent.
  write(Reg::DioMapping1, 0);

  write(Reg::Fifo, &packet, packet.length);
  set_op_mode(Bits::ModeTx);
}

}  // namespace Radio

/*------------------------------------------------------------------------------
 * Interrupt handler
 */
ISR(INT6_vect) {
  auto irq2{read(Reg::IrqFlags2)};
  if (irq2 & Bits::PayloadReady) {
    set_op_mode(Bits::ModeStdby);
    event_handler_.on_packet_sent();
  }

  if (irq2 & Bits::PacketSent) {
    set_op_mode(Bits::ModeStdby);
    event_handler_.on_packet_sent();
  }
}