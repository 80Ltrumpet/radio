#include "usb.h"

#include <alloca.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "bootloader.h"
#include "cdc_types.h"
#include "usb_types.h"

/*------------------------------------------------------------------------------
 * Constants, globals, and primitive subroutines
 */
namespace {

// Size of control and bulk endpoint FIFOs
constexpr uint8_t kFifoSize{64};

// Interface indices
enum : uint8_t {
  kInterfaceCdcAcm,
  kInterfaceCdcData,
  kInterfaceCount  // Must be last
};

// Endpoint indices
enum : uint8_t {
  kEndpointControl,
  kEndpointCdcCtrlAcm,
  kEndpointCdcDataOut,
  kEndpointCdcDataIn,
  kEndpointCount  // Must be last
};

// Easier-to-read CDC data endpoint aliases
constexpr auto kEndpointCdcRx{kEndpointCdcDataOut};
constexpr auto kEndpointCdcTx{kEndpointCdcDataIn};

// String indices
enum : uint8_t {
  kStringIdLanguage,
  kStringIdManufacturer,
  kStringIdProduct,
  kStringIdSerialNumber,
  kStringIdCount  // Must be last
};

PROGMEM const char kStringLanguage_P[]{'\11', '\4'};  // English
PROGMEM const char kStringManufacturer_P[]{USB_MANUFACTURER};
PROGMEM const char kStringProduct_P[]{USB_PRODUCT};
PROGMEM const char kStringSerialNumber_P[]{USB_SERIAL_NUMBER};
const char* kStringTable[kStringIdCount]{
    kStringLanguage_P,
    kStringManufacturer_P,
    kStringProduct_P,
    kStringSerialNumber_P,
};

// Device Descriptor
PROGMEM const UsbDeviceDesc kDeviceDescriptor_P{2,  // CDC
                                                0,
                                                0,
                                                kFifoSize,
                                                USB_VID,
                                                USB_PID,
                                                kStringIdManufacturer,
                                                kStringIdProduct,
                                                kStringIdSerialNumber,
                                                1};

PROGMEM const struct {
  const UsbConfigDesc config_desc{
      67, kInterfaceCount, 1, 0, UsbConfigDesc::kAttrSelfPowered, 500};

  const UsbInterfaceDesc acm_intf_desc{kInterfaceCdcAcm, 0, 1, 2, 2, 0, 0};
  const CdcHeaderFuncDesc cdc_hdr_desc{};
  const CdcCallMgmtFuncDesc cdc_cm_desc{
      CdcCallMgmtFuncDesc::kCapHandlesCallManagement, 1};
  const CdcAcmFuncDesc cdc_acm_desc{CdcAcmFuncDesc::kCapLineCoding |
                                    CdcAcmFuncDesc::kCapSendBreak};
  const CdcUnion2FuncDesc cdc_union_desc{0, 1};
  const UsbEndpointDesc acm_ep_desc{
      kEndpointCdcCtrlAcm | UsbEndpointDesc::kAddrIn,
      UsbEndpointDesc::kAttrTransferInterrupt, 16, 64};

  const UsbInterfaceDesc data_intf_desc{kInterfaceCdcData, 0, 2, 10, 0, 0, 0};
  const UsbEndpointDesc data_out_ep_desc{
      kEndpointCdcDataOut | UsbEndpointDesc::kAddrOut,
      UsbEndpointDesc::kAttrTransferBulk, kFifoSize, 0};
  const UsbEndpointDesc data_in_ep_desc{
      kEndpointCdcDataIn | UsbEndpointDesc::kAddrIn,
      UsbEndpointDesc::kAttrTransferBulk, kFifoSize, 0};
} kConfiguration_P{};
static_assert(sizeof(kConfiguration_P) == 67,
              "USB configuration blob has an unexpected size");

uint8_t g_wdtcsr_save{};
uint8_t g_configuration{};
CdcLineInfo g_cdc_line_info{};

FILE g_stream{};

void enable_clock() {
  // Power the internal regulator.
  UHWCON |= _BV(UVREGE);
  // Freeze the clock and enable USB.
  USBCON = _BV(USBE) | _BV(FRZCLK);
  // Enable the PLL.
  PLLCSR = (PLLCSR & ~_BV(PINDIV)) | _BV(PLLE);
  // Wait for the PLL to lock.
  while (!(PLLCSR & _BV(PLOCK)))
    ;
  // Start the clock and enable the VBUS pad.
  USBCON = (USBCON & ~_BV(FRZCLK)) | _BV(OTGPADE);
  // Attach internal pull-up for full-speed mode.
  UDCON = 0;
}

// TODO: This is possibly unused.
void disable_clock() {
  // Freeze the clock and disable the VBUS pad.
  USBCON = (USBCON | _BV(FRZCLK)) & ~_BV(OTGPADE);
  // Stop the PLL.
  PLLCSR = ~_BV(PLLE);
}

inline void wait_txin() {
  while (!(UEINTX & _BV(TXINI)))
    ;
}

inline void wait_rxout() {
  while (!(UEINTX & _BV(RXOUTI)))
    ;
}

// Returns true if RX is marked complete.
inline bool wait_in_or_out() {
  while (!(UEINTX & (_BV(TXINI) | _BV(RXOUTI))))
    ;
  return !(UEINTX & _BV(RXOUTI));
}

inline void ack_txin() { UEINTX = ~_BV(TXINI); }

inline void ack_rxout() { UEINTX = ~_BV(RXOUTI); }

inline void release_txin() {
  UEINTX = _BV(RWAL) | _BV(NAKOUTI) | _BV(RXSTPI) | _BV(STALLEDI);
}

// Reads FIFO data into the given buffer.
inline void recv_raw(volatile void* buffer, uint8_t length) {
  auto bytes{reinterpret_cast<volatile uint8_t*>(buffer)};
  while (length--) *bytes++ = UEDATX;
}

// Writes buffer data into the FIFO.
inline void send_raw(const void* buffer, uint8_t length) {
  auto bytes{reinterpret_cast<const uint8_t*>(buffer)};
  while (length--) UEDATX = *bytes++;
}

// Writes program space data into the FIFO.
inline void send_raw_P(const void* buffer, uint8_t length) {
  auto bytes{reinterpret_cast<const uint8_t*>(buffer)};
  while (length--) UEDATX = pgm_read_byte(bytes++);
}

// Reads control data that may require multiple transactions.
void recv_control(void* buffer, uint8_t length) {
  auto bytes{reinterpret_cast<uint8_t*>(buffer)};
  for (uint8_t i{0}; i < length;) {
    auto rlen{length > kFifoSize ? kFifoSize : length};
    wait_rxout();
    recv_raw(bytes + i, rlen);
    ack_rxout();
    ack_txin();
    i += rlen;
  }
}

// Writes control data that may require multiple transactions.
bool send_control(const void* buffer, uint8_t length, uint16_t limit,
                  bool pgm = false) {
  auto bytes{reinterpret_cast<const uint8_t*>(buffer)};
  uint8_t wlen{length < limit ? length : limit};
  uint8_t plen;  // Packet length

  do {
    if (!wait_in_or_out()) return false;
    plen = wlen < kFifoSize ? wlen : kFifoSize;
    if (pgm) {
      send_raw_P(bytes, plen);
    } else {
      send_raw(bytes, plen);
    }
    ack_txin();
    wlen -= plen;
  } while (wlen > 0 || plen == kFifoSize);

  return true;
}

}  // namespace

/*------------------------------------------------------------------------------
 * EpLock
 *
 * Similar to ATOMIC_BLOCK(ATOMIC_RESTORESTATE) while allowing the use of flow
 * control statements (break/continue) within loops. This "lock" sets EPNUM to
 * the requested endpoint index.
 */

class EpLock final {
 public:
  EpLock(uint8_t ep) : sreg_{SREG}, ep_{ep} {
    cli();
    UENUM = ep;
  }

  ~EpLock() { unlock(); }

  void lock() {
    if (!locked_) {
      sreg_ = SREG;
      cli();
      UENUM = ep_;
      locked_ = true;
    }
  }

  void unlock() {
    if (locked_) {
      SREG = sreg_;
      locked_ = false;
    }
  }

 private:
  uint8_t sreg_;
  uint8_t ep_;
  bool locked_{true};
};

/*------------------------------------------------------------------------------
 * Usb public static methods
 */

void Usb::Init() {
  g_configuration = 0;

  enable_clock();

  // Enable end-of-reset and start-of-frame interrupts.
  UDIEN = _BV(EORSTE) | _BV(SOFE);

  // Set up stdout/stderr for printf-related functions.
  fdev_setup_stream(&g_stream, PutChar, nullptr, _FDEV_SETUP_WRITE);
  stdout = &g_stream;
  stderr = &g_stream;

  #if 1 // DEBUG
  DDRC |= _BV(DDC7);
  PORTC &= ~_BV(PORTC7);
  #endif
}

bool Usb::GetChar(char& c) {
  if (g_configuration == 0) return false;
  EpLock lock{kEndpointCdcRx};
  if (!(UEINTX & _BV(RWAL))) return false;
  c = UEDATX;
  if (!(UEINTX & _BV(RWAL))) {
    UEINTX = _BV(NAKINI) | _BV(RWAL) | _BV(RXSTPI) | _BV(STALLEDI) | _BV(TXINI);
  }
  return true;
}

/*------------------------------------------------------------------------------
 * Usb private static methods
 */

int Usb::PutChar(char c, FILE* stream) {
  if (g_configuration == 0) return _FDEV_ERR;
  if (c == '\n' && PutChar('\r', stream) != 0) return _FDEV_ERR;

  static constexpr uint8_t kTimeoutMs{25};
  static auto timed_out{false};

  EpLock lock{kEndpointCdcTx};

  // Don't wait again if the previous call timed out.
  if (timed_out) {
    if (!(UEINTX & _BV(RWAL))) return _FDEV_ERR;
    timed_out = false;
  }

  // Busy-wait based on the frame count.
  const uint8_t timeout{UDFNUML + kTimeoutMs};
  while (!(UEINTX & _BV(RWAL))) {
    lock.unlock();
    if (UDFNUML == timeout) {
      timed_out = true;
      return _FDEV_ERR;
    }
    // Check if we got un-configured.
    if (g_configuration == 0) return _FDEV_ERR;
    lock.lock();
  }

  UEDATX = c;
  if (!(UEINTX & _BV(RWAL))) {
    release_txin();
  }

  return 0;
}

/*------------------------------------------------------------------------------
 * USB request-handling utilities
 */
namespace {

// Sends a properly-formatted string descriptor for the indexed string.
bool send_string_descriptor(const UsbSetupData& setup) {
  if (setup.valueL >= kStringIdCount) return false;
  auto addr_P{kStringTable[setup.valueL]};
  uint8_t str_len{strlen_P(addr_P)};
  auto desc_len{UsbStringDesc::GetLength(str_len)};

  // Allocate the descriptor on the stack.
  auto storage{alloca(desc_len)};
  auto desc{new (storage) UsbStringDesc{str_len}};

  // Copy the string from program space into the descriptor.
  for (uint8_t i{}; i < str_len; ++i) {
    desc->wString[i] = pgm_read_byte(addr_P + i);
  }

  return send_control(desc, desc_len, setup.wLength);
}

// Sends the descriptor according to the setup request.
bool send_descriptor(const UsbSetupData& setup) {
  switch (setup.valueH) {
    case UsbDesc::kTypeDevice:
      return send_control(&kDeviceDescriptor_P, sizeof(kDeviceDescriptor_P),
                          setup.wLength, true);
    case UsbDesc::kTypeConfig:
      return send_control(&kConfiguration_P, sizeof(kConfiguration_P),
                          setup.wLength, true);
    case UsbDesc::kTypeString:
      return send_string_descriptor(setup);
    default:
      break;
  }

  return false;
}

// Configures an endpoint based on its descriptor.
void configure_endpoint(const uint8_t* desc) {
  UsbEndpointDesc ep;
  memcpy_P(&ep, desc, sizeof(ep));
  const auto type{ep.bmAttributes & UsbEndpointDesc::kAttrTransferInterrupt};
  const auto size{(static_cast<uint16_t>(ep.wMaxPacketSizeH) << 8) |
                  ep.wMaxPacketSizeL};

  // Compute EPSIZE based on the max packet size.
  constexpr uint8_t kMaxEpsize{(_BV(EPSIZE2) | _BV(EPSIZE1) >> EPSIZE0)};
  uint8_t epsize{};
  for (uint16_t bsize{8}; bsize < size && epsize < kMaxEpsize;
       bsize <<= 1, ++epsize)
    ;
  epsize <<= EPSIZE0;

  UENUM = ep.bEndpointAddress & (_BV(UENUM_2 + 1) - 1);
  UECONX = _BV(EPEN);
  UECFG0X = ((ep.bEndpointAddress & UsbEndpointDesc::kAddrIn) >> 7) |
            (type << EPTYPE0);
  if (type == UsbEndpointDesc::kAttrTransferBulk) {
    UECFG1X = epsize | _BV(EPBK0) | _BV(ALLOC);
  } else {
    UECFG1X = epsize | _BV(ALLOC);
  }
}

// Configures endpoints according to their descriptors.
void configure_endpoints() {
  auto config{reinterpret_cast<const uint8_t*>(&kConfiguration_P)};
  // Traverse the configuration, looking for endpoint descriptors.
  for (uint8_t i{}; i < sizeof(kConfiguration_P);) {
    auto length{pgm_read_byte(config + i)};
    auto type{pgm_read_byte(config + (i + 1))};
    if (type == UsbDesc::kTypeEndpoint) {
      configure_endpoint(config + i);
    }
    i += length;
  }
  UERST = 0x7e;
  UERST = 0;
}

// Handles a standard request.
bool handle_standard_request(const UsbSetupData& setup) {
  bool success{true};
  switch (setup.bRequest) {
    case UsbSetupData::kReqGetStatus:
      wait_txin();
      // TODO: Add support for endpoint halt.
      UEDATX = 0;
      UEDATX = 0;
      ack_txin();
      break;
    case UsbSetupData::kReqSetAddress:
      ack_txin();
      wait_txin();
      // Data sheet section 22.7: "ADDEN and UADD shall not be written at the
      // same time."
      // ... Oh well!
      UDADDR = setup.valueL | _BV(ADDEN);
      break;
    case UsbSetupData::kReqGetDescriptor:
      success = send_descriptor(setup);
      break;
    case UsbSetupData::kReqGetConfiguration:
      wait_txin();
      // There is only one configuration.
      UEDATX = 1;
      ack_txin();
      break;
    case UsbSetupData::kReqSetConfiguration:
      if (setup.requestType.recipient == UsbSetupData::kRecipDevice) {
        ack_txin();
        configure_endpoints();
        g_configuration = setup.valueL;
      } else {
        // Bad request.
        success = false;
      }
      break;
    default:
      success = false;
      break;
  }

  return success;
}

bool handle_cdc_acm_interface_request(const UsbSetupData& setup) {
  if (setup.requestType.direction == UsbSetupData::kDirDeviceToHost) {
    if (setup.bRequest == kCdcReqGetLineCoding) {
      return send_control(&g_cdc_line_info.coding, CdcLineInfo::Coding::kSize,
                          setup.wLength);
    }
  } else {  // kDirHostToDevice
    switch (setup.bRequest) {
      case kCdcReqSetLineCoding:
        recv_control(&g_cdc_line_info.coding, CdcLineInfo::Coding::kSize);
        return true;
      case kCdcReqSetControlLineState:
        g_cdc_line_info.state = setup.valueL;
        wait_txin();
        ack_txin();

        // Auto-reset into the bootloader is triggered by closing the port
        // after it is opened at 1200 bps. To do this, start the watchdog with
        // a relatively long period.

        const auto p_boot_key{
            reinterpret_cast<uint16_t*>(Bootloader::kMagicKeyAddr)};
        const auto p_ram_end{reinterpret_cast<uint16_t*>(RAMEND - 1)};
        auto p_magic_key{p_boot_key};

        if (p_magic_key != p_ram_end &&
            pgm_read_word(FLASHEND - 1) == Bootloader::kNewLufaSignature) {
          p_magic_key = p_ram_end;
        }

        if (g_cdc_line_info.coding.dwDTERate == 1200 &&
            !(g_cdc_line_info.state & _BV(0))) {
          // Backup the RAM location for the magic key if we aren't using a
          // newer bootloader and it hasn't already been overwritten.
          if (p_boot_key != p_ram_end && p_magic_key != p_ram_end &&
              *p_magic_key != Bootloader::kMagicKey) {
            *p_ram_end = *p_magic_key;
          }

          // Store the boot key.
          *p_magic_key = Bootloader::kMagicKey;

          #if 1 // DEBUG
          PORTC |= _BV(PORTC7);
          #endif

          // Save the watchdog state in case the reset is aborted.
          g_wdtcsr_save = WDTCSR;
          wdt_enable(WDTO_120MS);
        } else if (*p_magic_key == Bootloader::kMagicKey) {
          // If the data rate was set to something besides 1200 bps, cancel
          // the watchdog.
          wdt_reset();
          WDTCSR |= _BV(WDCE) | _BV(WDE);
          WDTCSR = g_wdtcsr_save;

          #if 1 // DEBUG
          PORTC |= _BV(PORTC7);
          #endif

          // If a backup was necessary (see above), restore it.
          if (p_boot_key != p_ram_end && p_magic_key != p_ram_end) {
            *p_magic_key = *p_ram_end;
          } else {
            *p_magic_key = 0;
          }
        }
        return true;
      case kCdcReqSendBreak:
        // Succesfully do nothing.
        ack_txin();
        return true;
      default:
        break;
    }
  }

  return false;
}

bool handle_class_or_interface_request(const UsbSetupData& setup) {
  // Only CDC ACM interface requests are supported.
  if (setup.requestType.type == UsbSetupData::kTypeClass &&
      setup.requestType.recipient == UsbSetupData::kRecipInterface &&
      setup.wIndex == kInterfaceCdcAcm) {
    return handle_cdc_acm_interface_request(setup);
  }

  return false;
}

}  // namespace

// General interrupt
ISR(USB_GEN_vect) {
  auto udint{UDINT};
  UDINT &= ~udint;

  // End Of Reset
  if (udint & _BV(EORSTI)) {
    g_configuration = 0;
    UENUM = kEndpointControl;
    UECONX = _BV(EPEN);                                  // Enable endpoint 0
    UECFG0X = 0;                                         // Control type
    UECFG1X = _BV(EPSIZE1) | _BV(EPSIZE0) | _BV(ALLOC);  // 64B, single bank
    UEIENX = _BV(RXSTPE);                                // SETUP interrupt
  }

  // Start Of Frame (occurs every millisecond)
  if ((udint & _BV(SOFI)) && g_configuration != 0) {
    UENUM = kEndpointCdcTx;
    if (UEBCX > 0) {   // If there's something in the FIFO...
      release_txin();  // ... send it and switch banks.
    }
  }
}

// Endpoint interrupt
ISR(USB_COM_vect) {
  // Endpoint interrupts are only enabled for endpoint 0.
  UENUM = kEndpointControl;

  // We only care about SETUP interrupts.
  if (!(UEINTX & _BV(RXSTPI))) return;

  // Read the setup data.
  UsbSetupData setup;
  recv_raw(&setup, sizeof(setup));

  // Clear the interrupt.
  UEINTX = ~(_BV(RXSTPI) | _BV(RXOUTI) | _BV(TXINI));

  // Handle the request.
  bool success{true};
  if (setup.requestType.type == UsbSetupData::kTypeStandard) {
    success = handle_standard_request(setup);
  } else {
    success = handle_class_or_interface_request(setup);
  }

  // If the request failed, stall.
  if (!success) {
    UECONX = _BV(STALLRQ) | _BV(EPEN);
  }
}