#pragma once

#include <stdint.h>

namespace rfm69hcw {

// Register address definitions from Section 6.1, Table 23 of the data sheet
struct Reg final {
#define ADDR_T static constexpr uint8_t
  ADDR_T Fifo{0x00};
  ADDR_T OpMode{0x01};
  ADDR_T DataModul{0x02};
  ADDR_T BitrateMsb{0x03};
  ADDR_T BitrateLsb{0x04};
  ADDR_T FdevMsb{0x05};
  ADDR_T FdevLsb{0x06};
  ADDR_T FrfMsb{0x07};
  ADDR_T FrfMid{0x08};
  ADDR_T FrfLsb{0x09};
  ADDR_T Osc1{0x0a};
  ADDR_T AfcCtrl{0x0b};
  ADDR_T Listen1{0x0d};
  ADDR_T Listen2{0x0e};
  ADDR_T Listen3{0x0f};
  ADDR_T Version{0x10};
  ADDR_T PaLevel{0x11};
  ADDR_T PaRamp{0x12};
  ADDR_T Ocp{0x13};
  ADDR_T Lna{0x18};
  ADDR_T RxBw{0x19};
  ADDR_T AfcBw{0x1a};
  ADDR_T OokPeak{0x1b};
  ADDR_T OokAvg{0x1c};
  ADDR_T OokFix{0x1d};
  ADDR_T AfcFei{0x1e};
  ADDR_T AfcMsb{0x1f};
  ADDR_T AfcLsb{0x20};
  ADDR_T FeiMsb{0x21};
  ADDR_T FeiLsb{0x22};
  ADDR_T RssiConfig{0x23};
  ADDR_T RssiValue{0x24};
  ADDR_T DioMapping1{0x25};
  ADDR_T DioMapping2{0x26};
  ADDR_T IrqFlags1{0x27};
  ADDR_T IrqFlags2{0x28};
  ADDR_T RssiThresh{0x29};
  ADDR_T RxTimeout1{0x2a};
  ADDR_T RxTimeout2{0x2b};
  ADDR_T PreambleMsb{0x2c};
  ADDR_T PreambleLsb{0x2d};
  ADDR_T SyncConfig{0x2e};
  ADDR_T SyncValue{0x2f};
  // ... SyncValue8{0x36};
  ADDR_T PacketConfig1{0x37};
  ADDR_T PayloadLength{0x38};
  ADDR_T NodeAdrs{0x39};
  ADDR_T BroadcastAdrs{0x3a};
  ADDR_T AutoModes{0x3b};
  ADDR_T FifoThresh{0x3c};
  ADDR_T PacketConfig2{0x3d};
  ADDR_T AesKey{0x3e};
  // ... AesKey16{0x4d};
  ADDR_T Temp1{0x4e};
  ADDR_T Temp2{0x4f};
  ADDR_T TestLna{0x58};
  ADDR_T TestPa1{0x5a};
  ADDR_T TestPa2{0x5c};
  ADDR_T TestDagc{0x6f};
  ADDR_T TestAfc{0x71};
#undef ADDR_T

  // Multi-byte register lengths
  static constexpr uint8_t kSyncValueLength{8};
  static constexpr uint8_t kAesKeyLength{16};
};

// Register reset values from Section 6.1, Table 23 of the data sheet
struct Reset final {
#define VAL_T static constexpr uint8_t
  VAL_T Fifo{0x00};
  VAL_T OpMode{0x04};
  VAL_T DataModul{0x00};
  VAL_T BitrateMsb{0x1a};
  VAL_T BitrateLsb{0x0b};
  VAL_T FdevMsb{0x00};
  VAL_T FdevLsb{0x52};
  VAL_T FrfMsb{0xe4};
  VAL_T FrfMid{0xc0};
  VAL_T FrfLsb{0x00};
  VAL_T Osc1{0x41};
  VAL_T AfcCtrl{0x00};
  VAL_T Listen1{0x92};
  VAL_T Listen2{0xf5};
  VAL_T Listen3{0x20};
  VAL_T Version{0x24};
  VAL_T PaLevel{0x9f};
  VAL_T PaRamp{0x09};
  VAL_T Ocp{0x1a};
  VAL_T Lna{0x08};
  VAL_T RxBw{0x86};
  VAL_T AfcBw{0x8a};
  VAL_T OokPeak{0x40};
  VAL_T OokAvg{0x80};
  VAL_T OokFix{0x06};
  VAL_T AfcFei{0x10};
  VAL_T AfcMsb{0x00};
  VAL_T AfcLsb{0x00};
  VAL_T FeiMsb{0x00};
  VAL_T FeiLsb{0x00};
  VAL_T RssiConfig{0x02};
  VAL_T RssiValue{0xff};
  VAL_T DioMapping1{0x00};
  VAL_T DioMapping2{0x05};
  VAL_T IrqFlags1{0x80};
  VAL_T IrqFlags2{0x00};
  VAL_T RssiThresh{0xff};
  VAL_T RxTimeout1{0x00};
  VAL_T RxTimeout2{0x00};
  VAL_T PreambleMsb{0x00};
  VAL_T PreambleLsb{0x03};
  VAL_T SyncConfig{0x98};
  VAL_T SyncValueX{0x00};
  VAL_T PacketConfig1{0x10};
  VAL_T PayloadLength{0x40};
  VAL_T NodeAdrs{0x00};
  VAL_T BroadcastAdrs{0x00};
  VAL_T AutoModes{0x00};
  VAL_T FifoThresh{0x0f};
  VAL_T PacketConfig2{0x02};
  VAL_T AesKeyX{0x00};
  VAL_T Temp1{0x01};
  VAL_T Temp2{0x00};
  VAL_T TestLna{0x1b};
  VAL_T TestPa1{0x55};
  VAL_T TestPa2{0x70};
  VAL_T TestDagc{0x00};
  VAL_T TestAfc{0x00};
#undef VAL_T
};

// Register default/recommended values from Section 6.1, Table 23 in the data
// sheet (where defined)
struct Default final {
#define VAL_T static constexpr uint8_t
  VAL_T Lna{0x88};
  VAL_T RxBw{0x55};
  VAL_T AfcBw{0x8b};
  VAL_T DioMapping2{0x07};
  VAL_T RssiThresh{0xe4};
  VAL_T SyncValueX{0x01};
  VAL_T FifoThresh{0x8f};
  VAL_T TestDagc{0x30};
#undef VAL_T
};

/*------------------------------------------------------------------------------
 * Register bit fields and values from Section 6 in the data sheet
 *
 * Defining bit fields and their semantics in this way has several advantages:
 *  1. Obviates the _BV() macro.
 *  2. Avoids manual bit shifting in most cases (fields and values are defined
 *     "in place").
 *  3. Gives semantics to multi-bit values.
 *
 * These advantages are gained while preserving the potential for optimal code.
 */

// OpMode
constexpr uint8_t SequencerOff{0x80};
constexpr uint8_t ListenOn{0x40};
constexpr uint8_t ListenAbort{0x20};
constexpr uint8_t Mode{0x1c};
enum : uint8_t {
  ModeSleep,
  ModeStdby = 0x04,
  ModeFs = 0x08,
  ModeTx = 0x0c,
  ModeRx = 0x10,
};

// DataModul
constexpr uint8_t DataMode{0x60};
enum : uint8_t {
  DataModePacket,
  DataModeContWithBitSync = 0x40,
  DataModeContNoBitSync = 0x60,
};
constexpr uint8_t ModulationType{0x18};
enum : uint8_t {
  ModulationTypeFsk,
  ModulationTypeOok = 0x08,
};
// ModulationShaping semantics depend on ModulationType.
constexpr uint8_t ModulationShaping{0x03};
enum : uint8_t {
  ModulationShapingFskNone,
  ModulationShapingFskBt1p0,
  ModulationShapingFskBt0p5,
  ModulationShapingFskBt0p3,
};
enum : uint8_t {
  ModulationShapingOokNone,
  ModulationShapingOokCutoffBr,
  ModulationShapingOokCutoff2Br,
};

// Osc1
constexpr uint8_t RcCalStart{0x80};
constexpr uint8_t RcCalDone{0x40};

// AfcCtrl
constexpr uint8_t AfcLowBetaOn{0x20};

// Listen1
constexpr uint8_t ListenResolIdle{0xc0};
enum : uint8_t {
  ListenResolIdle64us = 0x40,
  ListenResolIdle4100us = 0x80,
  ListenResolIdle262ms = 0xc0,
};
constexpr uint8_t ListenResolRx{0x30};
enum : uint8_t {
  ListenResolRx64us = 0x10,
  ListenResolRx4100us = 0x20,
  ListenResolRx262ms = 0x30,
};
constexpr uint8_t ListenCriteria{0x08};
constexpr uint8_t ListenEnd{0x06};
enum : uint8_t {
  ListenEndStay,
  ListenEndMode = 0x02,
  ListenEndIdle = 0x04,
};

// PaLevel
constexpr uint8_t Pa0On{0x80};
constexpr uint8_t Pa1On{0x40};
constexpr uint8_t Pa2On{0x20};
// OutputPower semantics depend on PaXOn and other registers. See sections 3.3.6
// and 3.3.7 in the data sheet.
constexpr uint8_t OutputPower{0x1f};

// PaRamp
constexpr uint8_t PaRamp{0x0f};
enum : uint8_t {
  PaRamp3400us,
  PaRamp2000us,
  PaRamp1000us,
  PaRamp500us,
  PaRamp250us,
  PaRamp125us,
  PaRamp100us,
  PaRamp62us,
  PaRamp50us,
  PaRamp40us,
  PaRamp31us,
  PaRamp25us,
  PaRamp20us,
  PaRamp15us,
  PaRamp12us,
  PaRamp10us,
};

// Ocp
constexpr uint8_t OcpOn{0x10};
constexpr uint8_t OcpTrim{0x0f};
constexpr uint8_t OcpTrimToMa(uint8_t ocp) { return 45 + 5 * (ocp & OcpTrim); }
constexpr uint8_t OcpTrimFromMa(uint8_t ma) {
  return ((ma < 45 ? 45 : ma > 120 ? 120 : ma) - 45) / 5;
}

// Lna
constexpr uint8_t LnaZin{0x80};
enum : uint8_t {
  LnaZin50Ohms,
  LnaZin200Ohms = 0x80,
};
constexpr uint8_t LnaCurrentGain{0x38};
enum : uint8_t {
  LnaCurrentGain0db = 0x08,
  LnaCurrentGain6db = 0x10,
  LnaCurrentGain12db = 0x18,
  LnaCurrentGain24db = 0x20,
  LnaCurrentGain36db = 0x28,
  LnaCurrentGain48db = 0x30,
};
constexpr uint8_t LnaGainSelect{0x07};
enum : uint8_t {
  LnaGainSelectAgc,
  LnaGainSelect0db,
  LnaGainSelect6db,
  LnaGainSelect12db,
  LnaGainSelect24db,
  LnaGainSelect36db,
  LnaGainSelect48db,
};

// RxBw and AfcBw
constexpr uint8_t DccFreq{0xe0};
constexpr uint8_t DccFreq_{5};
constexpr uint8_t RxBwMant{0x18};
enum : uint8_t {
  RxBwMant16,
  RxBwMant20 = 0x08,
  RxBwMant24 = 0x10,
};
constexpr uint8_t RxBwExp{0x07};

// OokPeak
constexpr uint8_t OokThreshType{0xc0};
enum : uint8_t {
  OokThreshTypeFixed,
  OokThreshTypePeak = 0x40,
  OokThreshTypeAverage = 0x80,
};
constexpr uint8_t OokPeakThreshStep{0x38};
enum : uint8_t {
  OokPeakThreshStep0p5,
  OokPeakThreshStep1p0 = 0x08,
  OokPeakThreshStep1p5 = 0x10,
  OokPeakThreshStep2p0 = 0x18,
  OokPeakThreshStep3p0 = 0x20,
  OokPeakThreshStep4p0 = 0x28,
  OokPeakThreshStep5p0 = 0x30,
  OokPeakThreshStep6p0 = 0x38,
};
constexpr uint8_t OokPeakThreshDec{0x07};
enum : uint8_t {
  OokPeakThreshDecEvery1,
  OokPeakThreshDecEvery2,
  OokPeakThreshDecEvery4,
  OokPeakThreshDecEvery8,
  OokPeakThreshDec2Per,
  OokPeakThreshDec4Per,
  OokPeakThreshDec8Per,
  OokPeakThreshDec16Per,
};

// OokAvg
constexpr uint8_t OokAverageThreshFilt{0xc0};
enum : uint8_t {
  OokAverageThreshFiltDiv32Pi,
  OokAverageThreshFiltDiv8Pi = 0x40,
  OokAverageThreshFiltDiv4Pi = 0x80,
  OokAverageThreshFiltDiv2Pi = 0xc0,
};

// AfcFei
constexpr uint8_t FeiDone{0x40};
constexpr uint8_t FeiStart{0x20};
constexpr uint8_t AfcDone{0x10};
constexpr uint8_t AfcAutoclearOn{0x08};
constexpr uint8_t AfcAutoOn{0x04};
constexpr uint8_t AfcClear{0x02};
constexpr uint8_t AfcStart{0x01};

// RssiConfig
constexpr uint8_t RssiDone{0x02};
constexpr uint8_t RssiStart{0x01};

// DioMapping1
constexpr uint8_t Dio0Mapping{0xc0};
constexpr uint8_t Dio0Mapping_{6};
constexpr uint8_t Dio1Mapping{0x30};
constexpr uint8_t Dio1Mapping_{4};
constexpr uint8_t Dio2Mapping{0x0c};
constexpr uint8_t Dio2Mapping_{2};
constexpr uint8_t Dio3Mapping{0x03};

// DioMapping2
constexpr uint8_t Dio4Mapping{0xc0};
constexpr uint8_t Dio4Mapping_{6};
constexpr uint8_t Dio5Mapping{0x30};
constexpr uint8_t Dio5Mapping_{4};
constexpr uint8_t ClkOut{0x07};
enum : uint8_t {
  ClkOutFxoscDiv1,
  ClkOutFxoscDiv2,
  ClkOutFxoscDiv4,
  ClkOutFxoscDiv8,
  ClkOutFxoscDiv16,
  ClkOutFxoscDiv32,
  ClkOutRc,
  ClkOutOff,
};

// IrqFlags1
constexpr uint8_t ModeReady{0x80};
constexpr uint8_t RxReady{0x40};
constexpr uint8_t TxReady{0x20};
constexpr uint8_t PllLock{0x10};
constexpr uint8_t Rssi{0x08};
constexpr uint8_t Timeout{0x04};
constexpr uint8_t AutoMode{0x02};
constexpr uint8_t SyncAddressMatch{0x01};

// IrqFlags2
constexpr uint8_t FifoFull{0x80};
constexpr uint8_t FifoNotEmpty{0x40};
constexpr uint8_t FifoLevel{0x20};
constexpr uint8_t FifoOverrun{0x10};
constexpr uint8_t PacketSent{0x08};
constexpr uint8_t PayloadReady{0x04};
constexpr uint8_t CrcOk{0x02};

// SyncConfig
constexpr uint8_t SyncOn{0x80};
constexpr uint8_t FifoFillCondition{0x40};
constexpr uint8_t SyncSize{0x38};
constexpr uint8_t SyncSize_{3};
constexpr uint8_t SyncTol{0x07};

// PacketConfig1
constexpr uint8_t PacketFormat{0x80};
enum : uint8_t {
  PacketFormatFixed,
  PacketFormatVariable = 0x80,
};
constexpr uint8_t DcFree{0x60};
enum : uint8_t {
  DcFreeNone,
  DcFreeManchester = 0x20,
  DcFreeWhitening = 0x40,
};
constexpr uint8_t CrcOn{0x10};
constexpr uint8_t CrcAutoClearOff{0x08};
constexpr uint8_t AddressFiltering{0x06};
enum : uint8_t {
  AddressFilteringOff,
  AddressFilteringNode = 0x02,
  AddressFilteringBroadcast = 0x04,
};

// AutoModes
constexpr uint8_t EnterCondition{0xe0};
enum : uint8_t {
  EnterConditionNone,
  EnterConditionFifoNotEmptyRising = 0x20,
  EnterConditionFifoLevelRising = 0x40,
  EnterConditionCrcOkRising = 0x60,
  EnterConditionPayloadReadyRising = 0x80,
  EnterConditionSyncAddressRising = 0xa0,
  EnterConditionPacketSentRising = 0xc0,
  EnterConditionFifoNotEmptyFalling = 0xe0,
};
constexpr uint8_t ExitCondition{0x1c};
enum : uint8_t {
  ExitConditionNone,
  ExitConditionFifoNotEmptyFalling = 0x04,
  ExitConditionFifoLevelOrTimeoutRising = 0x08,
  ExitConditionCrcOkOrTimeoutRising = 0x0c,
  ExitConditionPayloadReadyOrTimeoutRising = 0x10,
  ExitConditionSyncAddressOrTimeoutRising = 0x14,
  ExitConditionPacketSentRising = 0x18,
  ExitConditionTimeoutRising = 0x1c,
};
constexpr uint8_t IntermediateMode{0x03};
enum : uint8_t {
  IntermediateModeSleep,
  IntermediateModeStdby,
  IntermediateModeRx,
  IntermediateModeTx,
};

// FifoThresh
constexpr uint8_t TxStartCondition{0x80};
constexpr uint8_t FifoThreshold{0x7f};

// PacketConfig2
constexpr uint8_t InterPacketRxDelay{0xf0};
constexpr uint8_t InterPacketRxDelay_{4};
constexpr uint8_t RestartRx{0x04};
constexpr uint8_t AutoRxRestartOn{0x02};
constexpr uint8_t AesOn{0x01};

// Temp1
constexpr uint8_t TempMeasStart{0x08};
constexpr uint8_t TempMeasRunning{0x04};

// Temp2
constexpr int8_t TempValueToCelsius(uint8_t temp2) { return 85 - temp2; }

// TestLna
enum : uint8_t {
  SensitivityBoostNormal = 0x1b,
  SensitivityBoostHigh = 0x2d,
};

// TestPa1
enum : uint8_t {
  Pa20dBm1NormalRx = 0x55,
  Pa20dBm1Plus20 = 0x5d,
};

// TestPa2
enum : uint8_t {
  Pa20dBm2NormalRx = 0x70,
  Pa20dBm2Plus20 = 0x7c,
};

// TestDagc
enum : uint8_t {
  ContinuousDagcNormal,
  ContinuousDagcImprovedAfcLowBetaOn = 0x20,
  ContinuousDagcImprovedAfcLowBetaOff = 0x30,
};

}  // namespace rfm69hcw