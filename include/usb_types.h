#pragma once

#include <stdint.h>

struct UsbSetupData final {
  // bmRequestType : recipient
  enum : uint8_t {
    kRecipDevice,
    kRecipInterface,
    kRecipEndpoint,
    kRecipOther,
    kRecipReserved
  };

  // bmRequestType : type
  enum : uint8_t { kTypeStandard, kTypeClass, kTypeVendor, kTypeReserved };

  // bmRequestType : direction
  enum : uint8_t { kDirHostToDevice, kDirDeviceToHost };

  // bRequest
  enum : uint8_t {
    kReqGetStatus,
    kReqClearFeature,
    kReqSetFeature = 3,
    kReqSetAddress = 5,
    kReqGetDescriptor,
    kReqSetDescriptor,
    kReqGetConfiguration,
    kReqSetConfiguration,
    kReqGetInterface,
    kReqSetInterface,
    kReqSynchFrame
  };

  // CLEAR_FEATURE / SET_FEATURE
  enum : uint8_t {
    kFeatureEndpointHalt,
    kFeatureDeviceRemoteWakeup,
    kFeatureTestMode
  };

  union {
    struct {
      uint8_t recipient : 5;
      uint8_t type : 2;
      uint8_t direction : 1;
    } requestType;
    uint8_t bmRequestType;
  };
  uint8_t bRequest;
  union {
    struct {
      uint8_t valueL;
      uint8_t valueH;
    };
    uint16_t wValue;
  };
  uint16_t wIndex;
  uint16_t wLength;
};
static_assert(sizeof(UsbSetupData) == 8, "USB setup data has incorrect size");

struct UsbDesc {
  // Standard descriptor types (that we use)
  enum : uint8_t {
    kTypeDevice = 1,
    kTypeConfig,
    kTypeString,
    kTypeInterface,
    kTypeEndpoint,
    kTypeDeviceQualifier,
    kTypeOtherSpeedConfig,
    kTypeInterfacePower,
  };

  constexpr UsbDesc(uint8_t length, uint8_t type)
      : bLength{length}, bDescriptorType{type} {}

  // All USB descriptors start with length and type fields.
  const uint8_t bLength;
  const uint8_t bDescriptorType;
};

struct UsbDeviceDesc final : public UsbDesc {
  constexpr UsbDeviceDesc(uint8_t clazz, uint8_t subclass, uint8_t protocol,
                          uint8_t max_packet_size, uint16_t vid, uint16_t pid,
                          uint8_t manufacturer, uint8_t product, uint8_t serial,
                          uint8_t configs)
      : UsbDesc{18, kTypeDevice},
        bDeviceClass{clazz},
        bDeviceSubclass{subclass},
        bDeviceProtocol{protocol},
        bMaxPacketSize{max_packet_size},
        idVendor{vid},
        idProduct{pid},
        iManufacturer{manufacturer},
        iProduct{product},
        iSerialNumber{serial},
        bNumConfigurations{configs} {}

  const uint16_t bcdUSB{0x0200};  // 2.0.0
  const uint8_t bDeviceClass;
  const uint8_t bDeviceSubclass;
  const uint8_t bDeviceProtocol;
  const uint8_t bMaxPacketSize;
  const uint16_t idVendor;
  const uint16_t idProduct;
  const uint16_t bcdDevice{0x0100};  // 1.0.0
  const uint8_t iManufacturer;
  const uint8_t iProduct;
  const uint8_t iSerialNumber;
  const uint8_t bNumConfigurations;
};
static_assert(sizeof(UsbDeviceDesc) == 18,
              "USB Device Descriptor size is incorrect");

struct UsbConfigDesc final : public UsbDesc {
  static constexpr uint8_t kAttrReserved{0x80};  // Must be set
  static constexpr uint8_t kAttrSelfPowered{0x40};
  static constexpr uint8_t kAttrRemoteWakeup{0x20};

  constexpr UsbConfigDesc(uint16_t total_length, uint8_t interfaces,
                          uint8_t index, uint8_t string, uint8_t attrs,
                          uint16_t max_power_ma)
      : UsbDesc{9, kTypeConfig},
        wTotalLengthL{total_length & 0xff},
        wTotalLengthH{total_length >> 8},
        bNumInterfaces{interfaces},
        bConfigurationValue{index},
        iConfiguration{string},
        bmAttributes{attrs | kAttrReserved},
        bMaxPower{(max_power_ma >>= 1) > UINT8_MAX
                      ? UINT8_MAX
                      : static_cast<uint8_t>(max_power_ma)} {}

  const uint8_t wTotalLengthL;
  const uint8_t wTotalLengthH;
  const uint8_t bNumInterfaces;
  const uint8_t bConfigurationValue;
  const uint8_t iConfiguration;
  const uint8_t bmAttributes;
  const uint8_t bMaxPower;
};
static_assert(sizeof(UsbConfigDesc) == 9,
              "USB Configuration Descriptor size is incorrect");

struct UsbStringDesc final : public UsbDesc {
 private:
  // This is needed to make the squiggles go away.
  using size_t = unsigned long long;

 public:
  constexpr UsbStringDesc(uint8_t str_len)
      : UsbDesc{GetLength(str_len), kTypeString} {}

  void* operator new(size_t, void* ptr) noexcept { return ptr; }

  // Computes the length of the descriptor based on the length of a string.
  static constexpr uint8_t GetLength(uint8_t str_len) {
    return 2 + str_len * 2;
  }

  char16_t wString[0]{};  // UNICODE encoded string
};

struct UsbInterfaceDesc final : public UsbDesc {
  constexpr UsbInterfaceDesc(uint8_t n, uint8_t alt, uint8_t eps, uint8_t clazz,
                             uint8_t subclass, uint8_t protocol, uint8_t string)
      : UsbDesc{9, kTypeInterface},
        bInterfaceNumber{n},
        bAlternateSetting{alt},
        bNumEndpoints{eps},
        bInterfaceClass{clazz},
        bInterfaceSubClass{subclass},
        bInterfaceProtocol{protocol},
        iInterface{string} {}

  const uint8_t bInterfaceNumber;
  const uint8_t bAlternateSetting;
  const uint8_t bNumEndpoints;
  const uint8_t bInterfaceClass;
  const uint8_t bInterfaceSubClass;
  const uint8_t bInterfaceProtocol;
  const uint8_t iInterface;
};
static_assert(sizeof(UsbInterfaceDesc) == 9,
              "USB Interface Descriptor size is incorrect");

struct UsbEndpointDesc final : public UsbDesc {
  static constexpr uint8_t kAddrOut{};
  static constexpr uint8_t kAddrIn{0x80};
  static constexpr uint8_t kAddrReserved{0x70};

  enum : uint8_t {
    kAttrTransferControl,
    kAttrTransferIsochronous,
    kAttrTransferBulk,
    kAttrTransferInterrupt
  };

  // For isochronous endpoints only
  static constexpr uint8_t kAttrSyncNone{};
  static constexpr uint8_t kAttrSyncAsynchronous{0x04};
  static constexpr uint8_t kAttrSyncAdaptive{0x08};
  static constexpr uint8_t kAttrSyncSynchronous{0x0c};
  static constexpr uint8_t kAttrUsageData{};
  static constexpr uint8_t kAttrUsageFeedback{0x10};
  static constexpr uint8_t kAttrUsageImplicitFeedbackData{0x20};
  static constexpr uint8_t kAttrUsageReserved{0x30};

  static constexpr uint8_t kAttrReserved{0xc0};

  static constexpr uint8_t kMaxPacketSize{0x7ff};
  // These only apply to high-speed isochronous/interrupt endpoints.
  static constexpr uint8_t kPlusOneTransaction{0x08};
  static constexpr uint8_t kPlusTwoTransactions{0x10};

  constexpr UsbEndpointDesc(uint8_t addr, uint8_t attrs,
                            uint16_t max_packet_size, uint8_t interval,
                            uint8_t plus = 0)
      : UsbDesc{7, kTypeEndpoint},
        bEndpointAddress{addr & ~kAddrReserved},
        bmAttributes{attrs & ~kAttrReserved},
        wMaxPacketSizeL{max_packet_size & 0xff},
        wMaxPacketSizeH{(max_packet_size >> 8) |
                        (plus == 0   ? 0
                         : plus == 1 ? kPlusOneTransaction
                                     : kPlusTwoTransactions)},
        bInterval{interval} {}

  UsbEndpointDesc() : UsbDesc{7, kTypeEndpoint} {}

  const uint8_t bEndpointAddress{};
  const uint8_t bmAttributes{};
  uint8_t wMaxPacketSizeL{};
  uint8_t wMaxPacketSizeH{};
  const uint8_t bInterval{};
};
static_assert(sizeof(UsbEndpointDesc) == 7,
              "USB Endpoint Descriptor size is incorrect");