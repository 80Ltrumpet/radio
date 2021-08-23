#pragma once

#include <stdint.h>

#include "vec3.h"

namespace lsm6dsox {

constexpr uint8_t kI2cAddr{0x6a};

// Register address definitions from Section 8, Table 21 of the data sheet
namespace Reg {

constexpr uint8_t FUNC_CFG_ACCESS{0x01};
constexpr uint8_t PIN_CTRL{0x02};
constexpr uint8_t S4S_TPH_L{0x04};
constexpr uint8_t S4S_TPH_H{0x05};
constexpr uint8_t S4S_RR{0x06};
constexpr uint8_t FIFO_CTRL1{0x07};
constexpr uint8_t FIFO_CTRL2{0x08};
constexpr uint8_t FIFO_CTRL3{0x09};
constexpr uint8_t FIFO_CTRL4{0x0a};
constexpr uint8_t COUNTER_BDR_REG1{0x0b};
constexpr uint8_t COUNTER_BDR_REG2{0x0c};
constexpr uint8_t INT1_CTRL{0x0d};
constexpr uint8_t INT2_CTRL{0x0e};
constexpr uint8_t WHO_AM_I{0x0f};
constexpr uint8_t CTRL1_XL{0x10};
constexpr uint8_t CTRL2_G{0x11};
constexpr uint8_t CTRL3_C{0x12};
constexpr uint8_t CTRL4_C{0x13};
constexpr uint8_t CTRL5_C{0x14};
constexpr uint8_t CTRL6_C{0x15};
constexpr uint8_t CTRL7_G{0x16};
constexpr uint8_t CTRL8_XL{0x17};
constexpr uint8_t CTRL9_XL{0x18};
constexpr uint8_t CTRL10_C{0x19};
constexpr uint8_t ALL_INT_SRC{0x1a};
constexpr uint8_t WAKE_UP_SRC{0x1b};
constexpr uint8_t TAP_SRC{0x1c};
constexpr uint8_t D6D_SRC{0x1d};
constexpr uint8_t STATUS_REG{0x1e};
constexpr uint8_t OUT_TEMP_L{0x20};
constexpr uint8_t OUT_TEMP_H{0x21};
constexpr uint8_t OUTX_L_G{0x22};
constexpr uint8_t OUTX_H_G{0x23};
constexpr uint8_t OUTY_L_G{0x24};
constexpr uint8_t OUTY_H_G{0x25};
constexpr uint8_t OUTZ_L_G{0x26};
constexpr uint8_t OUTZ_H_G{0x27};
constexpr uint8_t OUTX_L_A{0x28};
constexpr uint8_t OUTX_H_A{0x29};
constexpr uint8_t OUTY_L_A{0x2a};
constexpr uint8_t OUTY_H_A{0x2b};
constexpr uint8_t OUTZ_L_A{0x2c};
constexpr uint8_t OUTZ_H_A{0x2d};
constexpr uint8_t EMB_FUNC_STATUS_MAINPAGE{0x35};
constexpr uint8_t FSM_STATUS_A_MAINPAGE{0x36};
constexpr uint8_t FSM_STATUS_B_MAINPAGE{0x37};
constexpr uint8_t MLC_STATUS_MAINPAGE{0x38};
constexpr uint8_t STATUS_MASTER_MAINPAGE{0x39};
constexpr uint8_t FIFO_STATUS1{0x3a};
constexpr uint8_t FIFO_STATUS2{0x3b};
constexpr uint8_t TIMESTAMP0{0x40};
constexpr uint8_t TIMESTAMP1{0x41};
constexpr uint8_t TIMESTAMP2{0x42};
constexpr uint8_t TIMESTAMP3{0x43};
constexpr uint8_t UI_STATUS_REG_OIS{0x49};
constexpr uint8_t UI_OUTX_L_G_OIS{0x4a};
constexpr uint8_t UI_OUTX_H_G_OIS{0x4b};
constexpr uint8_t UI_OUTY_L_G_OIS{0x4c};
constexpr uint8_t UI_OUTY_H_G_OIS{0x4d};
constexpr uint8_t UI_OUTZ_L_G_OIS{0x4e};
constexpr uint8_t UI_OUTZ_H_G_OIS{0x4f};
constexpr uint8_t UI_OUTX_L_A_OIS{0x50};
constexpr uint8_t UI_OUTX_H_A_OIS{0x51};
constexpr uint8_t UI_OUTY_L_A_OIS{0x52};
constexpr uint8_t UI_OUTY_H_A_OIS{0x53};
constexpr uint8_t UI_OUTZ_L_A_OIS{0x54};
constexpr uint8_t UI_OUTZ_H_A_OIS{0x55};
constexpr uint8_t TAP_CFG0{0x56};
constexpr uint8_t TAP_CFG1{0x57};
constexpr uint8_t TAP_CFG2{0x58};
constexpr uint8_t TAP_THS_6D{0x59};
constexpr uint8_t INT_DUR2{0x5a};
constexpr uint8_t WAKE_UP_THS{0x5b};
constexpr uint8_t WAKE_UP_DUR{0x5c};
constexpr uint8_t FREE_FALL{0x5d};
constexpr uint8_t MD1_CFG{0x5e};
constexpr uint8_t MD2_CFG{0x5f};
constexpr uint8_t S4S_ST_CMD_CODE{0x60};
constexpr uint8_t S4S_DT_REG{0x61};
constexpr uint8_t I3C_BUS_AVB{0x62};
constexpr uint8_t INTERNAL_FREQ_FINE{0x63};
constexpr uint8_t UI_INT_OIS{0x6f};
constexpr uint8_t UI_CTRL1_OIS{0x70};
constexpr uint8_t UI_CTRL2_OIS{0x71};
constexpr uint8_t UI_CTRL3_OIS{0x72};
constexpr uint8_t X_OFS_USR{0x73};
constexpr uint8_t Y_OFS_USR{0x74};
constexpr uint8_t Z_OFS_USR{0x75};
constexpr uint8_t FIFO_DATA_OUT_TAG{0x78};
constexpr uint8_t FIFO_DATA_OUT_X_L{0x79};
constexpr uint8_t FIFO_DATA_OUT_X_H{0x7a};
constexpr uint8_t FIFO_DATA_OUT_Y_L{0x7b};
constexpr uint8_t FIFO_DATA_OUT_Y_H{0x7c};
constexpr uint8_t FIFO_DATA_OUT_Z_L{0x7d};
constexpr uint8_t FIFO_DATA_OUT_Z_H{0x7e};

// Non-zero reset (power-on) register values
namespace Reset {

constexpr uint8_t PIN_CTRL{0x3f};
constexpr uint8_t WHO_AM_I{0x6c};
constexpr uint8_t CTRL3_C{0x04};
constexpr uint8_t CTRL9_XL{0xe0};

}  // namespace Reset

}  // namespace Reg

// Register bit fields and values from Section 9 in the data sheet
namespace Bits {

// FUNC_CFG_ACCESS
constexpr uint8_t FUNC_CFG_ACCESS{0x80};
constexpr uint8_t SHUB_REG_ACCESS{0x40};
constexpr uint8_t OIS_CTRL_FROM_UI{0x01};

// PIN_CTRL
constexpr uint8_t OIS_PU_DIS{0x80};
constexpr uint8_t SDO_PU_EN{0x40};

// S4S_TPH_L
constexpr uint8_t TPH_H_SEL{0x80};
constexpr uint8_t TPH_L{0x7f};

// S4S_RR
constexpr uint8_t RR{0x03};
enum RR_value : uint8_t {
  RR_2_11,
  RR_2_12,
  RR_2_13,
  RR_2_14,
};

// FIFO_CTRL2
constexpr uint8_t STOP_ON_WTM{0x80};
constexpr uint8_t FIFO_COMPR_RT_EN{0x40};
constexpr uint8_t ODRCHG_EN{0x10};
constexpr uint8_t UNCOPTR_RATE{0x06};
enum UNCOPTR_RATE_value : uint8_t {
  UNCOPTR_RATE_NONE,
  UNCOPTR_RATE_8 = 0x02,
  UNCOPTR_RATE_16 = 0x04,
  UNCOPTR_RATE_32 = 0x06,
};
constexpr uint8_t WTM8{0x01};

// FIFO_CTRL3
constexpr uint8_t BDR_GY{0xf0};
constexpr uint8_t BDR_GY_{4};
constexpr uint8_t BDR_XL{0x0f};
enum BDR_value : uint8_t {
  BDR_0,
  BDR_12P5,
  BDR_26,
  BDR_52,
  BDR_104,
  BDR_208,
  BDR_417,
  BDR_833,
  BDR_1667,
  BDR_3333,
  BDR_6667,
  BDR_6P5_G_1P6_XL,
};

// FIFO_CTRL4
constexpr uint8_t DEC_TS_BATCH{0xc0};
enum DEC_TS_BATCH_value : uint8_t {
  DEC_TS_BATCH_NONE,
  DEC_TS_BATCH_1 = 0x40,
  DEC_TS_BATCH_8 = 0x80,
  DEC_TS_BATCH_32 = 0xc0,
};
constexpr uint8_t ODR_T_BATCH{0x30};
enum ODR_T_BATCH_value : uint8_t {
  ODR_T_BATCH_NONE,
  ODR_T_BATCH_1P6 = 0x10,
  ODR_T_BATCH_12P5 = 0x20,
  ODR_T_BATCH_52 = 0x30,
};
constexpr uint8_t FIFO_MODE{0x07};
enum FIFO_MODE_value : uint8_t {
  FIFO_MODE_BYPASS,
  FIFO_MODE_STOP_ON_FULL,
  FIFO_MODE_CONT2FIFO = 0x03,
  FIFO_MODE_BYPASS2CONT,
  FIFO_MODE_CONTINUOUS = 0x06,
  FIFO_MODE_BYPASS2FIFO,
};

// COUNTER_BDR_REG1
constexpr uint8_t DATAREADY_PULSED{0x80};
constexpr uint8_t RST_COUNTER_BDR{0x40};
constexpr uint8_t TRIG_COUNTER_BDR{0x20};
constexpr uint8_t CNT_BDR_TH_H{0x07};

// INT1_CTRL
constexpr uint8_t DEN_DRDY_FLAG{0x80};
constexpr uint8_t INT1_CNT_BDR{0x40};
constexpr uint8_t INT1_FIFO_FULL{0x20};
constexpr uint8_t INT1_FIFO_OVR{0x10};
constexpr uint8_t INT1_FIFO_TH{0x08};
constexpr uint8_t INT1_BOOT{0x04};
constexpr uint8_t INT1_DRDY_G{0x02};
constexpr uint8_t INT1_DRDY_XL{0x01};

// INT2_CTRL
constexpr uint8_t INT2_CNT_BDR{0x40};
constexpr uint8_t INT2_FIFO_FULL{0x20};
constexpr uint8_t INT2_FIFO_OVR{0x10};
constexpr uint8_t INT2_FIFO_TH{0x08};
constexpr uint8_t INT2_DRDY_TEMP{0x04};
constexpr uint8_t INT2_DRDY_G{0x02};
constexpr uint8_t INT2_DRDY_XL{0x01};

// CTRL1_XL
constexpr uint8_t ODR_XL{0xf0};
enum ODR_XL_value : uint8_t {
  ODR_XL_OFF,
  ODR_XL_1P6_12P5 = 0xb0,
  ODR_XL_12P5 = 0x10,
  ODR_XL_26 = 0x20,
  ODR_XL_52 = 0x30,
  ODR_XL_104 = 0x40,
  ODR_XL_208 = 0x50,
  ODR_XL_416 = 0x60,
  ODR_XL_833 = 0x70,
  ODR_XL_1P66K = 0x80,
  ODR_XL_3P33K = 0x90,
  ODR_XL_6P66K = 0xa0,
};
constexpr uint8_t FS_XL{0x0c};
enum FS_XL_value : uint8_t {
  FS_XL_2G,
  FS_XL_16G_2G = 0x04,
  FS_XL_4G = 0x08,
  FS_XL_8G = 0x0c,
};
constexpr uint8_t LPF2_XL_EN{0x02};

// CTRL2_G
constexpr uint8_t ODR_G{0xf0};
enum ODR_G_value : uint8_t {
  ODR_G_OFF,
  ODR_G_12P5 = 0x10,
  ODR_G_26 = 0x20,
  ODR_G_52 = 0x30,
  ODR_G_104 = 0x40,
  ODR_G_208 = 0x50,
  ODR_G_416 = 0x60,
  ODR_G_833 = 0x70,
  ODR_G_1P66K = 0x80,
  ODR_G_3P33K = 0x90,
  ODR_G_6P66K = 0xa0,
};
constexpr uint8_t FS_G{0x0c};
enum FS_G_value : uint8_t {
  FS_G_250DPS,
  FS_G_500DPS = 0x04,
  FS_G_1000DPS = 0x08,
  FS_G_2000DPS = 0x0c,
};
constexpr uint8_t FS_125{0x02};

// CTRL3_C
constexpr uint8_t BOOT{0x80};
constexpr uint8_t BDU{0x40};
constexpr uint8_t H_LACTIVE{0x20};
constexpr uint8_t PP_OD{0x10};
constexpr uint8_t SIM{0x08};
constexpr uint8_t IF_INC{0x04};
constexpr uint8_t SW_RESET{0x01};

// CTRL4_C
constexpr uint8_t SLEEP_G{0x40};
constexpr uint8_t INT2_ON_INT1{0x20};
constexpr uint8_t DRDY_MASK{0x08};
constexpr uint8_t I2C_DISABLE{0x04};
constexpr uint8_t LPF1_SEL_G{0x02};

// CTRL5_C
constexpr uint8_t XL_ULP_EN{0x80};
constexpr uint8_t ROUNDING{0x60};
enum ROUNDING_value : uint8_t {
  ROUNDING_NONE,
  ROUNDING_XL = 0x20,
  ROUNDING_G = 0x40,
  ROUNDING_BOTH = 0x60,
};
constexpr uint8_t ROUNDING_STATUS{0x10};
constexpr uint8_t ST_G{0x0c};
enum ST_G_value : uint8_t {
  ST_G_NORMAL,
  ST_G_POS = 0x04,
  ST_G_NEG = 0x0c,
};
constexpr uint8_t ST_XL{0x03};
enum ST_XL_value : uint8_t {
  ST_XL_NORMAL,
  ST_XL_POS,
  ST_XL_NEG,
};

// CTRL6_C
// Refer to Table 66.
constexpr uint8_t TRIG_EN{0x80};
// Refer to Table 66.
constexpr uint8_t LVL1_EN{0x40};
// Refer to Table 66.
constexpr uint8_t LVL2_EN{0x20};
constexpr uint8_t XL_HM_MODE{0x10};
constexpr uint8_t USR_OFF_W{0x08};
// Refer to Table 67.
constexpr uint8_t FTYPE{0x07};

// CTRL7_G
constexpr uint8_t G_HM_MODE{0x80};
constexpr uint8_t HP_EN_G{0x40};
constexpr uint8_t HPM_G{0x30};
enum HPM_G_value : uint8_t {
  HPM_G_16MIHZ,
  HPM_G_65MIHZ = 0x10,
  HPM_G_260MIHZ = 0x20,
  HPM_G_1P04HZ = 0x30,
};
constexpr uint8_t OIS_ON_EN{0x40};
constexpr uint8_t USR_OFF_ON_OUT{0x02};
constexpr uint8_t OIS_ON{0x01};

// CTRL8_XL
// Refer to Table 72.
constexpr uint8_t HPCF_XL{0xe0};
enum HPCF_XL_value : uint8_t {
  HPCF_XL_DIV4,
  HPCF_XL_DIV10 = 0x20,
  HPCF_XL_DIV20 = 0x40,
  HPCF_XL_DIV45 = 0x60,
  HPCF_XL_DIV100 = 0x80,
  HPCF_XL_DIV200 = 0xa0,
  HPCF_XL_DIV400 = 0xc0,
  HPCF_XL_DIV800 = 0xe0,
};
constexpr uint8_t HP_REF_MODE_XL{0x10};
constexpr uint8_t FASTSETTL_MODE_XL{0x08};
// Refer to Table 72.
constexpr uint8_t HP_SLOPE_XL_EN{0x04};
constexpr uint8_t XL_FS_MODE{0x02};
constexpr uint8_t LOW_PASS_ON_6D{0x01};

// CTRL9_XL
constexpr uint8_t DEN_X{0x80};
constexpr uint8_t DEN_Y{0x40};
constexpr uint8_t DEN_Z{0x20};
constexpr uint8_t DEN_XL_G{0x10};
constexpr uint8_t DEN_XL_EN{0x08};
constexpr uint8_t DEN_LH{0x04};
constexpr uint8_t I3C_DISABLE{0x02};

// CTRL10_C
constexpr uint8_t TIMESTAMP_EN{0x20};

// ALL_INT_SRC
constexpr uint8_t TIMESTAMP_ENDCOUNT{0x80};
constexpr uint8_t SLEEP_CHANGE_IA{0x20};
constexpr uint8_t D6D_IA{0x10};
constexpr uint8_t DOUBLE_TAP{0x08};
constexpr uint8_t SINGLE_TAP{0x04};
constexpr uint8_t WU_IA{0x02};
constexpr uint8_t FF_IA{0x01};

// WAKE_UP_SRC
// Three bits have the same names as different bits in ALL_INT_SRC. It is not
// clear from the data sheet if these are shadow bits or if they mean something
// completely different. See Section 9.26.
constexpr uint8_t SLEEP_STATE{0x10};
constexpr uint8_t X_WU{0x04};
constexpr uint8_t Y_WU{0x02};
constexpr uint8_t Z_WU{0x01};

// TAP_SRC
// Two bits have the same names as different bits in ALL_INT_SRC. It is not
// clear from the data sheet if these are shadow bits or if they mean something
// completely different. See Section 9.27.
constexpr uint8_t TAP_IA{0x40};
constexpr uint8_t TAP_SIGN{0x08};
constexpr uint8_t X_TAP{0x04};
constexpr uint8_t Y_TAP{0x02};
constexpr uint8_t Z_TAP{0x01};

// D6D_SRC
// One bit has the same name as a different bit in ALL_INT_SRC. It is not clear
// from the data sheet if this is a shadow bit or if it means something
// completely different. See Section 9.28.
constexpr uint8_t DEN_DRDY{0x80};
constexpr uint8_t ZH{0x20};
constexpr uint8_t ZL{0x10};
constexpr uint8_t YH{0x08};
constexpr uint8_t YL{0x04};
constexpr uint8_t XH{0x02};
constexpr uint8_t XL{0x01};

// STATUS_REG
constexpr uint8_t TDA{0x04};
constexpr uint8_t GDA{0x02};
constexpr uint8_t XLDA{0x01};

// EMB_FUNC_STATUS_MAINPAGE
constexpr uint8_t IS_FMS_LC{0x80};
constexpr uint8_t IS_SIGMOT{0x20};
constexpr uint8_t IS_TILT{0x10};
constexpr uint8_t IS_STEP_DET{0x08};

// STATUS_MASTER_MAINPAGE
constexpr uint8_t WR_ONCE_DONE{0x80};
constexpr uint8_t SLAVE3_NACK{0x40};
constexpr uint8_t SLAVE2_NACK{0x20};
constexpr uint8_t SLAVE1_NACK{0x10};
constexpr uint8_t SLAVE0_NACK{0x08};
constexpr uint8_t SENS_HUB_ENDOP{0x01};

// FIFO_STATUS2
constexpr uint8_t FIFO_WTM_IA{0x80};
constexpr uint8_t FIFO_OVR_IA{0x40};
constexpr uint8_t FIFO_FULL_IA{0x20};
constexpr uint8_t COUNTER_BDR_IA{0x10};
constexpr uint8_t FIFO_OVER_LATCHED{0x08};
constexpr uint8_t DIFF_FIFO_H{0x03};

// FIFO_STATUS1 + FIFO_STATUS2
constexpr uint16_t DIFF_FIFO{0x03ff};

// UI_STATUS_REG_OIS
// Two bits have the same names as the equivalent bits in STATUS_REG. It is not
// clear from the data sheet if these are shadow bits or if they mean something
// completely different. See Section 9.45.
constexpr uint8_t GYRO_SETTLING{0x04};

// TAP_CFG0
constexpr uint8_t INT_CLR_ON_READ{0x40};
constexpr uint8_t SLEEP_STATUS_ON_INT{0x20};
constexpr uint8_t SLOPE_FDS{0x10};
constexpr uint8_t TAP_X_EN{0x08};
constexpr uint8_t TAP_Y_EN{0x04};
constexpr uint8_t TAP_Z_EN{0x02};
constexpr uint8_t LIR{0x01};

// TAP_CFG1
constexpr uint8_t TAP_PRIORITY{0xe0};
enum TAP_PRIORITY_value : uint8_t {
  TAP_PRIORITY_XYZ,
  TAP_PRIORITY_YXZ = 0x20,
  TAP_PRIORITY_XZY = 0x40,
  TAP_PRIORITY_ZYX = 0x60,
  TAP_PRIORITY_YZX = 0xa0,
  TAP_PRIORITY_ZXY = 0xc0,
};
constexpr uint8_t TAP_THS_X{0x1f};

// TAP_CFG2
constexpr uint8_t INTERRUPTS_ENABLE{0x80};
constexpr uint8_t INACT_EN{0x60};
enum INACT_EN_value : uint8_t {
  INACT_EN_NEITHER_CHANGE,
  INACT_EN_G_NO_CHANGE = 0x20,
  INACT_EN_G_SLEEP = 0x40,
  INACT_EN_G_POWER_DOWN = 0x60,
};
constexpr uint8_t TAP_THS_Y{0x1f};

// TAP_THS_6D
constexpr uint8_t D4D_EN{0x80};
constexpr uint8_t SIXD_THS{0x60};
enum SIXD_THS_value : uint8_t {
  SIXD_THS_80,
  SIXD_THS_70 = 0x20,
  SIXD_THS_60 = 0x40,
  SIXD_THS_50 = 0x60,
};
constexpr uint8_t TAP_THS_Z{0x1f};

// INT_DUR2
constexpr uint8_t DUR{0xf0};
constexpr uint8_t DUR_{4};
constexpr uint8_t QUIET{0x0c};
constexpr uint8_t QUIET_{2};
constexpr uint8_t SHOCK{0x03};

// WAKE_UP_THS
constexpr uint8_t SINGLE_DOUBLE_TAP{0x80};
constexpr uint8_t USR_OFF_ON_WU{0x40};
constexpr uint8_t WK_THS{0x3f};

// WAKE_UP_DUR
constexpr uint8_t FF_DUR5{0x80};
constexpr uint8_t WAKE_DUR{0x60};
constexpr uint8_t WAKE_DUR_{5};
constexpr uint8_t WAKE_THS_W{0x10};
constexpr uint8_t SLEEP_DUR{0x0f};

// FREE_FALL
constexpr uint8_t FF_DUR{0xf8};
constexpr uint8_t FF_DUR_{3};
constexpr uint8_t FF_THS{0x07};
enum FF_THS_value : uint8_t {
  FF_THS_156MG,
  FF_THS_219MG,
  FF_THS_250MG,
  FF_THS_312MG,
  FF_THS_344MG,
  FF_THS_406MG,
  FF_THS_469MG,
  FF_THS_500MG,
};

// MD1_CFG
constexpr uint8_t INT1_SLEEP_CHANGE{0x80};
constexpr uint8_t INT1_SINGLE_TAP{0x40};
constexpr uint8_t INT1_WU{0x20};
constexpr uint8_t INT1_FF{0x10};
constexpr uint8_t INT1_DOUBLE_TAP{0x08};
constexpr uint8_t INT1_6D{0x04};
constexpr uint8_t INT1_EMB_FUNC{0x02};
constexpr uint8_t INT1_SHUB{0x01};

// MD2_CFG
constexpr uint8_t INT2_SLEEP_CHANGE{0x80};
constexpr uint8_t INT2_SINGLE_TAP{0x40};
constexpr uint8_t INT2_WU{0x20};
constexpr uint8_t INT2_FF{0x10};
constexpr uint8_t INT2_DOUBLE_TAP{0x08};
constexpr uint8_t INT2_6D{0x04};
constexpr uint8_t INT2_EMB_FUNC{0x02};
constexpr uint8_t INT2_TIMESTAMP{0x01};

// I3C_BUS_AVB
constexpr uint8_t I3C_BUS_AVB_SEL{0x18};
enum I3C_BUS_AVB_SEL_value : uint8_t {
  I3C_BUS_AVB_SEL_50US,
  I3C_BUS_AVB_SEL_2US = 0x08,
  I3C_BUS_AVB_SEL_1MS = 0x10,
  I3C_BUS_AVB_SEL_25MS = 0x18,
};
constexpr uint8_t PD_DIS_INT1{0x01};

// UI_INT_OIS
constexpr uint8_t INT2_DRDY_OIS{0x80};
constexpr uint8_t LVL2_OIS{0x40};
constexpr uint8_t DEN_LH_OIS{0x20};
constexpr uint8_t SPI2_READ_EN{0x08};

// UI_CTRL1_OIS
constexpr uint8_t LVL1_OIS{0x40};
constexpr uint8_t SIM_OIS{0x20};
constexpr uint8_t MODE4_EN{0x10};
constexpr uint8_t FS_G_OIS{0x0c};
enum FS_G_OIS_value : uint8_t {
  FS_G_OIS_250DPS,
  FS_G_OIS_500DPS = 0x04,
  FS_G_OIS_1000DPS = 0x08,
  FS_G_OIS_2000DPS = 0x0c,
};
constexpr uint8_t FS_125_OIS{0x02};
constexpr uint8_t OIS_EN_SPI2{0x01};

// UI_CTRL2_OIS
constexpr uint8_t HPM_OIS{0x30};
enum HPM_OIS_value : uint8_t {
  HPM_OIS_16MIHZ,
  HPM_OIS_65MIHZ = 0x10,
  HPM_OIS_260MIHZ = 0x20,
  HPM_OIS_1P04HZ = 0x30,
};
// Refer to Table 182.
constexpr uint8_t FTYPE_OIS{0x06};
constexpr uint8_t FTYPE_OIS_{1};
constexpr uint8_t HP_EN_OIS{0x01};

// UI_CTRL3_OIS
// Refer to Table 185.
constexpr uint8_t FS_XL_OIS{0xc0};
constexpr uint8_t FS_XL_OIS_{6};
// Refer to Table 186.
constexpr uint8_t FILTER_XL_CONF_OIS{0x38};
constexpr uint8_t FILTER_XL_CONF_OIS_{3};
constexpr uint8_t ST_OIS_CLAMPDIS{0x01};

// FIFO_DATA_OUT_TAG
constexpr uint8_t TAG_SENSOR{0xf8};
enum TAG_SENSOR_value : uint8_t {
  TAG_SENSOR_INVALID,
  TAG_SENSOR_GYRO_NC,
  TAG_SENSOR_ACCEL_NC,
  TAG_SENSOR_TEMPERATURE,
  TAG_SENSOR_TIMESTAMP,
  TAG_SENSOR_CFG_CHANGE,
  TAG_SENSOR_ACCEL_NC_T_2,
  TAG_SENSOR_ACCEL_NC_T_1,
  TAG_SENSOR_ACCEL_2XC,
  TAG_SENSOR_ACCEL_3XC,
  TAG_SENSOR_GYRO_NC_T_2,
  TAG_SENSOR_GYRO_NC_T_1,
  TAG_SENSOR_GYRO_2XC,
  TAG_SENSOR_GYRO_3XC,
  TAG_SENSOR_SHUB_SLAVE0,
  TAG_SENSOR_SHUB_SLAVE1,
  TAG_SENSOR_SHUB_SLAVE2,
  TAG_SENSOR_SHUB_SLAVE3,
  TAG_SENSOR_STEP_COUNTER,
  TAG_SENSOR_SHUB_NACK = 0x19,
};
constexpr uint8_t TAG_CNT{0x06};
constexpr uint8_t TAG_CNT_{1};
constexpr uint8_t TAG_PARITY{0x01};

}  // namespace Bits

// Encapsulates one packet of FIFO data.
struct FifoData final {
  union Tag {
    struct {
      const uint8_t parity : 1;
      const uint8_t counter : 2;
      const uint8_t sensor : 5;
    };
    const uint8_t value;
  };

  using Data = Vec3<int16_t>;

  const Data& data() const {
    return *reinterpret_cast<const Data*>(raw);
  }

  Tag tag;
  uint8_t raw[sizeof(Data)];
};
static_assert(sizeof(FifoData) == 7, "FifoData has incorrect size");

uint16_t ProcessFifo(void (*visitor)(const FifoData&), uint16_t limit = 0);

}  // namespace lsm6dsox