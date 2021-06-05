#pragma once

#include "usb_types.h"

// Supported CDC requests
enum : uint8_t {
  kCdcReqSetLineCoding = 0x20,
  kCdcReqGetLineCoding,
  kCdcReqSetControlLineState,
  kCdcReqSendBreak,
};

struct CdcLineInfo final {
  struct Coding final {
    static constexpr uint8_t kSize{7};
    uint32_t dwDTERate{57600};
    uint8_t bCharFormat{};
    uint8_t bParityType{};
    uint8_t bDataBits{};
  };

  Coding coding{};
  uint8_t state{};
};

// N specifies the length of data.
template <uint8_t N>
struct CdcFuncDesc : public UsbDesc {
  static constexpr uint8_t kTypeCdcInterface{0x24};

  // CDC functional descriptor subtypes (that we use)
  enum : uint8_t {
    kSubtypeHeader,
    kSubtypeCallMgmt,
    kSubtypeAcm,
    kSubtypeUnion = 6,
  };

  constexpr CdcFuncDesc(uint8_t subtype)
      : UsbDesc{3 + N, kTypeCdcInterface}, bDescriptorSubtype{subtype} {}

  const uint8_t bDescriptorSubtype;
  uint8_t data[N]{};
};

struct CdcHeaderFuncDesc : public CdcFuncDesc<2> {
  constexpr CdcHeaderFuncDesc(uint16_t bcd_cdc = 0x0110)
      : CdcFuncDesc{kSubtypeHeader} {
    data[0] = bcd_cdc & 0xff;
    data[1] = bcd_cdc >> 8;
  }
};
static_assert(sizeof(CdcHeaderFuncDesc) == 5,
              "CDC Header Functional Descriptor size is incorrect");

struct CdcUnion2FuncDesc : public CdcFuncDesc<2> {
  constexpr CdcUnion2FuncDesc(uint8_t control, uint8_t subordinate)
      : CdcFuncDesc{kSubtypeUnion} {
    data[0] = control;
    data[1] = subordinate;
  }
};
static_assert(sizeof(CdcUnion2FuncDesc) == 5,
              "CDC Union<2> Functional Descriptor size is incorrect");

struct CdcCallMgmtFuncDesc : public CdcFuncDesc<2> {
  static constexpr uint8_t kCapHandlesCallManagement{0x01};
  static constexpr uint8_t kCapManagesOverDataInterface{0x02};
  static constexpr uint8_t kCapReserved{0xfc};

  constexpr CdcCallMgmtFuncDesc(uint8_t caps, uint8_t data_intf)
      : CdcFuncDesc{kSubtypeCallMgmt} {
    data[0] = caps & ~kCapReserved;
    data[1] = data_intf;
  }
};
static_assert(sizeof(CdcCallMgmtFuncDesc) == 5,
              "CDC Call Management Functional Descriptor size is incorrect");

struct CdcAcmFuncDesc : public CdcFuncDesc<1> {
  static constexpr uint8_t kCapCommFeature{0x01};
  static constexpr uint8_t kCapLineCoding{0x02};
  static constexpr uint8_t kCapSendBreak{0x04};
  static constexpr uint8_t kCapNetworkConnection{0x08};
  static constexpr uint8_t kCapReserved{0xf0};

  constexpr CdcAcmFuncDesc(uint8_t caps) : CdcFuncDesc{kSubtypeAcm} {
    data[0] = caps & ~kCapReserved;
  }
};
static_assert(
    sizeof(CdcAcmFuncDesc) == 4,
    "CDC Abstract Control Management Functional Descriptor size is incorrect");