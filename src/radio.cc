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
#include "gpio.h"
#include "rfm69hcw.h"
#include "scheduler.h"
#include "spi.h"
#include "timer.h"

using namespace rfm69hcw;

namespace {

// The slave select and reset pins are board-specific.
#if defined(ARDUINO_AVR_MEGA2560)
Gpio ss_{PINL, 0};
Gpio rst_{PINL, 1};
Gpio int_{PINE, 5};
#elif defined(ARDUINO_AVR_FEATHER32U4)
Gpio ss_{PINB, 4};
Gpio rst_{PIND, 4};
Gpio int_{PINE, 6};
#else
#error "Unknown board type."
#endif

/*------------------------------------------------------------------------------
 * SPI
 */

void ss_select(bool select) {
  // SS is asserted low and has an external pull-up.
  if (select) {
    // Current drain
    ss_.out();
  } else {
    // External pull-up
    ss_.in();
  }
}

void ss_config() { ss_.clear(); }

const Spi spi_{ss_select, ss_config};

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

constexpr const uint8_t kSyncWords[]{RADIO_SYNC_WORDS};

uint8_t address_{Radio::kInvalidAddr};
Radio::Client client_{};

// Shadow register to optimize read-modify-write operations
uint8_t op_mode_{Reg::Reset::OpMode};

// Reads the RSSI in dBm.
int8_t read_rssi() {
  return -static_cast<int8_t>(read(Reg::RssiValue) >> 1);
}

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
  if (auto sync_config{read(Reg::SyncConfig)};
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
      0xcd,  // FdevLsb
      0x6c,  // FrfMsb
      0x40,  // FrfMid
      0x00,  // FrfLsb
  };
  write(Reg::DataModul, buffer, kBufLen);

  // Enable PA1 with +13dBm (required for reliable operation).
  write(Reg::PaLevel, Pa1On | OutputPower);

  // Set the channel filter bandwidth to 400 kHz with a 500 Hz DCC cutoff.
  buffer[0] = buffer[1] = (7 << DccFreq_) | RxBwMant20;
  write(Reg::RxBw, buffer, 2);

  // Use variable-length packets with DC-free whitening and CRC checks. Use
  // node and broadcast address filtering.
  write(Reg::PacketConfig1, PacketFormatVariable | DcFreeWhitening | CrcOn |
                                AddressFilteringBroadcast);

  // Read the radio address programmed into the EEPROM.
  Eeprom::Read(Eeprom::Data::RadioAddress, &address_);
  // If the node address is the same as the broadcast address, mark it invalid.
  if (address_ == Radio::kBroadcastAddr) address_ = Radio::kInvalidAddr;
  // Set the node and broadcast addresses.
  buffer[0] = address_;
  buffer[1] = Radio::kBroadcastAddr;
  write(Reg::NodeAdrs, buffer, 2);

  // Leave everything else at default values.
}

void set_op_mode(uint8_t mode) {
  if (auto op_mode{op_mode_}; (op_mode & Bits::Mode) != mode) {
    op_mode = (op_mode & ~Bits::Mode) | mode;
    write(Reg::OpMode, op_mode);
    // Wait for the mode to change.
    while (!(read(Reg::IrqFlags1) & Bits::ModeReady))
      ;
    op_mode_ = op_mode;
  }
}

}  // namespace

namespace Radio {

/*------------------------------------------------------------------------------
 * Public API
 */
void Init() {
#if defined(ARDUINO_AVR_MEGA2560)
  // Use the reset line as an output.
  rst_.set();
  rst_.out();

  // Pulse reset high for at least 100 microseconds.
  Timer::DelayUs(100);
  rst_.clear();
#elif defined(ARDUINO_AVR_FEATHER32U4)
  // Make sure the reset line is floating.
  rst_.in();
  rst_.clear();

  // Wait until the reset line is low.
  while (rst_.get())
    ;
#endif

  // Wait at least 10 milliseconds to ensure the chip is out of reset.
  Timer::DelayMs(10);

  // Configure the external interrupt (rising edge)
  int_.in();
  int_.clear();
#if defined(ARDUINO_AVR_MEGA2560)
  EICRB |= _BV(ISC51) | _BV(ISC50);
  EIMSK |= _BV(INT5);
#elif defined(ARDUINO_AVR_FEATHER32U4)
  EICRB |= _BV(ISC61) | _BV(ISC60);
  EIMSK |= _BV(INT6);
#endif

  set_op_mode(Bits::ModeStdby);
  set_defaults();
  configure();
}

uint8_t GetAddress() { return address_; }

void SetAddress(uint8_t addr) {
  Eeprom::Update(Eeprom::Data::RadioAddress, &addr);
  address_ = addr;
  write(Reg::NodeAdrs, address_);
}

void SetClient(Client&& client) {
  client_ = static_cast<Client&&>(client);
}

void Listen() {
  if ((op_mode_ & Bits::Mode) == Bits::ModeRx) return;
  set_op_mode(Bits::ModeRx);

  // Switch the interrupt source to PayloadReady.
  write(Reg::DioMapping1, 1 << Bits::Dio0Mapping_);
}

bool Receive(IFifoBuffer* buffer) {
  if (!(read(Reg::IrqFlags2) & Bits::FifoNotEmpty)) {
    return false;
  }

  // Small optimization to perform the entire read in a single transaction.
  Spi::Transaction spi{spi_};
  spi.write(Reg::Fifo | Spi::kAddrRead);
  // Read the packet length.
  spi.read(buffer->get(), 1);
  // Read the message.
  spi.read(buffer->bytes() + 1, *buffer->cbytes());

  return true;
}

bool Send(uint8_t dest, const void* data, uint8_t length) {
  // Can't send a packet that is longer than the FIFO.
  const auto total_length{length + sizeof(Header)};
  if (total_length > kFifoSize) return false;

  // NOTE: This assumes that only one task is responsible for sending packets.
  // Otherwise, we would have to ensure that we are not in transmit mode, first.
  set_op_mode(Bits::ModeStdby);

  // Switch the interrupt source to PacketSent.
  write(Reg::DioMapping1, 0);

  {
    // Small optimization to perform the entire write in a single transaction.
    Spi::Transaction spi{spi_};
    spi.write(Reg::Fifo | Spi::kAddrWrite);
    // The length byte is not included in the total.
    spi.write(total_length - 1);
    spi.write(dest);
    spi.write(GetAddress());
    spi.write(data, length);
  }

  set_op_mode(Bits::ModeTx);
  return true;
}

}  // namespace Radio

/*------------------------------------------------------------------------------
 * Interrupt handler
 */
#if defined(ARDUINO_AVR_MEGA2560)
ISR(INT5_vect) {
#elif defined(ARDUINO_AVR_FEATHER32U4)
ISR(INT6_vect) {
#endif
  auto irq2{read(Reg::IrqFlags2)};
  if (irq2 & Bits::PayloadReady) {
    auto rssi{read_rssi()};
    set_op_mode(Bits::ModeStdby);
    if (client_.on_payload_ready) client_.on_payload_ready(rssi);
  }

  if (irq2 & Bits::PacketSent) {
    // Exit Tx mode as quickly as possible to minimize power draw.
    set_op_mode(Bits::ModeStdby);
    if (client_.on_packet_sent) client_.on_packet_sent();
  }
}