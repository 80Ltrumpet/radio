#pragma once

#include <stdint.h>

namespace lis3mdl {

constexpr uint8_t kI2cAddr{0x1c};

// Register address definitions from Section 6, Table 16 of the data sheet
namespace Reg {

constexpr uint8_t WHO_AM_I{0x0f};
constexpr uint8_t CTRL1{0x20};
constexpr uint8_t CTRL2{0x21};
constexpr uint8_t CTRL3{0x22};
constexpr uint8_t CTRL4{0x23};
constexpr uint8_t CTRL5{0x24};
constexpr uint8_t STATUS{0x27};
constexpr uint8_t OUT_X_L{0x28};
constexpr uint8_t OUT_X_H{0x29};
constexpr uint8_t OUT_Y_L{0x2a};
constexpr uint8_t OUT_Y_H{0x2b};
constexpr uint8_t OUT_Z_L{0x2c};
constexpr uint8_t OUT_Z_H{0x2d};
constexpr uint8_t TEMP_OUT_L{0x2e};
constexpr uint8_t TEMP_OUT_H{0x2f};
constexpr uint8_t INT_CFG{0x30};
constexpr uint8_t INT_SRC{0x31};
constexpr uint8_t INT_THS_L{0x32};
constexpr uint8_t INT_THS_H{0x33};

// Non-zero reset (power-on) register values
namespace Reset {

constexpr uint8_t WHO_AM_I{0x3d};
constexpr uint8_t CTRL1{0x10};
constexpr uint8_t CTRL3{0x03};
constexpr uint8_t INT_CFG{0xe8};

}  // namespace Reset

}  // namespace Reg

// Register bit fields and values from Section 7 in the data sheet
namespace Bits {

// CTRL1
constexpr uint8_t TEMP_EN{0x80};
constexpr uint8_t OM{0x60};
enum OM_value : uint8_t {
  OM_LP,
  OM_MP = 0x20,
  OM_HP = 0x40,
  OM_UHP = 0x60,
};
constexpr uint8_t DO{0x1c};
enum DO_value : uint8_t {
  DO_0P625HZ,
  DO_1P25HZ = 0x04,
  DO_2P5HZ = 0x08,
  DO_5HZ = 0x0c,
  DO_10HZ = 0x10,
  DO_20HZ = 0x14,
  DO_40HZ = 0x18,
  DO_80HZ = 0x1c,
};
constexpr uint8_t FAST_ODR{0x02};
constexpr uint8_t ST{0x01};

// CTRL2
constexpr uint8_t FS{0x60};
enum FS_value : uint8_t {
  FS_4GAUSS,
  FS_8GAUSS = 0x20,
  FS_12GAUSS = 0x40,
  FS_16GAUSS = 0x60,
};
constexpr uint8_t REBOOT{0x08};
constexpr uint8_t SOFT_RST{0x04};

// CTRL3
constexpr uint8_t LP{0x20};
constexpr uint8_t SIM{0x04};
constexpr uint8_t MD{0x03};
enum MD_value : uint8_t {
  MD_CONTINUOUS,
  MD_SINGLE,
  MD_POWER_DOWN,
  // MD_POWER_DOWN_2,
};

// CTRL4
constexpr uint8_t OMZ{0x0c};
enum OMZ_value : uint8_t {
  OMZ_LP,
  OMZ_MP = 0x04,
  OMZ_HP = 0x08,
  OMZ_UHP = 0x0c,
};
constexpr uint8_t BLE{0x02};

// CTRL5
constexpr uint8_t FAST_READ{0x80};
constexpr uint8_t BDU{0x40};

// STATUS
constexpr uint8_t ZYXOR{0x80};
constexpr uint8_t ZOR{0x40};
constexpr uint8_t YOR{0x20};
constexpr uint8_t XOR{0x10};
constexpr uint8_t ZYXDA{0x08};
constexpr uint8_t ZDA{0x04};
constexpr uint8_t YDA{0x02};
constexpr uint8_t XDA{0x01};

// INT_CFG
constexpr uint8_t XIEN{0x80};
constexpr uint8_t YIEN{0x40};
constexpr uint8_t ZIEN{0x20};
constexpr uint8_t IEA{0x04};
constexpr uint8_t LIR{0x02};
constexpr uint8_t IEN{0x01};

// INT_SRC
constexpr uint8_t PTH_X{0x80};
constexpr uint8_t PTH_Y{0x40};
constexpr uint8_t PTH_Z{0x20};
constexpr uint8_t NTH_X{0x10};
constexpr uint8_t NTH_Y{0x08};
constexpr uint8_t NTH_Z{0x04};
constexpr uint8_t MROI{0x02};
constexpr uint8_t INT{0x01};

}  // namespace Bits

}  // namespace lis3mdl