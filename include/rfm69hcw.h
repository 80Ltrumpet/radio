#pragma once

#include <stdint.h>

namespace rfm69hcw {

constexpr uint8_t kFifoSize{66};

// Register address definitions from Section 6.1, Table 23 of the data sheet
namespace Reg {

constexpr uint8_t Fifo{0x00};
constexpr uint8_t OpMode{0x01};
constexpr uint8_t DataModul{0x02};
constexpr uint8_t BitrateMsb{0x03};
constexpr uint8_t BitrateLsb{0x04};
constexpr uint8_t FdevMsb{0x05};
constexpr uint8_t FdevLsb{0x06};
constexpr uint8_t FrfMsb{0x07};
constexpr uint8_t FrfMid{0x08};
constexpr uint8_t FrfLsb{0x09};
constexpr uint8_t Osc1{0x0a};
constexpr uint8_t AfcCtrl{0x0b};
constexpr uint8_t Listen1{0x0d};
constexpr uint8_t Listen2{0x0e};
constexpr uint8_t Listen3{0x0f};
constexpr uint8_t Version{0x10};
constexpr uint8_t PaLevel{0x11};
constexpr uint8_t PaRamp{0x12};
constexpr uint8_t Ocp{0x13};
constexpr uint8_t Lna{0x18};
constexpr uint8_t RxBw{0x19};
constexpr uint8_t AfcBw{0x1a};
constexpr uint8_t OokPeak{0x1b};
constexpr uint8_t OokAvg{0x1c};
constexpr uint8_t OokFix{0x1d};
constexpr uint8_t AfcFei{0x1e};
constexpr uint8_t AfcMsb{0x1f};
constexpr uint8_t AfcLsb{0x20};
constexpr uint8_t FeiMsb{0x21};
constexpr uint8_t FeiLsb{0x22};
constexpr uint8_t RssiConfig{0x23};
constexpr uint8_t RssiValue{0x24};
constexpr uint8_t DioMapping1{0x25};
constexpr uint8_t DioMapping2{0x26};
constexpr uint8_t IrqFlags1{0x27};
constexpr uint8_t IrqFlags2{0x28};
constexpr uint8_t RssiThresh{0x29};
constexpr uint8_t RxTimeout1{0x2a};
constexpr uint8_t RxTimeout2{0x2b};
constexpr uint8_t PreambleMsb{0x2c};
constexpr uint8_t PreambleLsb{0x2d};
constexpr uint8_t SyncConfig{0x2e};
constexpr uint8_t SyncValue{0x2f};
// ... SyncValue8{0x36};
constexpr uint8_t PacketConfig1{0x37};
constexpr uint8_t PayloadLength{0x38};
constexpr uint8_t NodeAdrs{0x39};
constexpr uint8_t BroadcastAdrs{0x3a};
constexpr uint8_t AutoModes{0x3b};
constexpr uint8_t FifoThresh{0x3c};
constexpr uint8_t PacketConfig2{0x3d};
constexpr uint8_t AesKey{0x3e};
// ... AesKey16{0x4d};
constexpr uint8_t Temp1{0x4e};
constexpr uint8_t Temp2{0x4f};
constexpr uint8_t TestLna{0x58};
constexpr uint8_t TestPa1{0x5a};
constexpr uint8_t TestPa2{0x5c};
constexpr uint8_t TestDagc{0x6f};
constexpr uint8_t TestAfc{0x71};

// Multi-byte register lengths
static constexpr uint8_t kSyncValueLength{8};
static constexpr uint8_t kAesKeyLength{16};

// Register reset values (if non-zero) from Section 6.1, Table 23 of the data
// sheet
namespace Reset {

constexpr uint8_t OpMode{0x04};
constexpr uint8_t BitrateMsb{0x1a};
constexpr uint8_t BitrateLsb{0x0b};
constexpr uint8_t FdevLsb{0x52};
constexpr uint8_t FrfMsb{0xe4};
constexpr uint8_t FrfMid{0xc0};
constexpr uint8_t Osc1{0x41};
constexpr uint8_t Listen1{0x92};
constexpr uint8_t Listen2{0xf5};
constexpr uint8_t Listen3{0x20};
constexpr uint8_t Version{0x24};
constexpr uint8_t PaLevel{0x9f};
constexpr uint8_t PaRamp{0x09};
constexpr uint8_t Ocp{0x1a};
constexpr uint8_t Lna{0x08};
constexpr uint8_t RxBw{0x86};
constexpr uint8_t AfcBw{0x8a};
constexpr uint8_t OokPeak{0x40};
constexpr uint8_t OokAvg{0x80};
constexpr uint8_t OokFix{0x06};
constexpr uint8_t AfcFei{0x10};
constexpr uint8_t RssiConfig{0x02};
constexpr uint8_t RssiValue{0xff};
constexpr uint8_t DioMapping2{0x05};
constexpr uint8_t IrqFlags1{0x80};
constexpr uint8_t RssiThresh{0xff};
constexpr uint8_t PreambleLsb{0x03};
constexpr uint8_t SyncConfig{0x98};
constexpr uint8_t PacketConfig1{0x10};
constexpr uint8_t PayloadLength{0x40};
constexpr uint8_t FifoThresh{0x0f};
constexpr uint8_t PacketConfig2{0x02};
constexpr uint8_t Temp1{0x01};
constexpr uint8_t TestLna{0x1b};
constexpr uint8_t TestPa1{0x55};
constexpr uint8_t TestPa2{0x70};

}  // namespace Reset

// Register default/recommended values from Section 6.1, Table 23 in the data
// sheet (where defined)
namespace Default {

constexpr uint8_t Lna{0x88};
constexpr uint8_t RxBw{0x55};
constexpr uint8_t AfcBw{0x8b};
constexpr uint8_t DioMapping2{0x07};
constexpr uint8_t RssiThresh{0xe4};
constexpr uint8_t SyncValueX{0x01};
constexpr uint8_t FifoThresh{0x8f};
constexpr uint8_t TestDagc{0x30};

}  // namespace Default

}  // namespace Reg

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

namespace Bits {

// OpMode
constexpr uint8_t SequencerOff{0x80};
constexpr uint8_t ListenOn{0x40};
constexpr uint8_t ListenAbort{0x20};
constexpr uint8_t Mode{0x1c};
enum OpModeMode : uint8_t {
  ModeSleep,
  ModeStdby = 0x04,
  ModeFs = 0x08,
  ModeTx = 0x0c,
  ModeRx = 0x10,
};

// DataModul
constexpr uint8_t DataMode{0x60};
enum DataModulDataMode : uint8_t {
  DataModePacket,
  DataModeContWithBitSync = 0x40,
  DataModeContNoBitSync = 0x60,
};
constexpr uint8_t ModulationType{0x18};
enum DataModulModulationType : uint8_t {
  ModulationTypeFsk,
  ModulationTypeOok = 0x08,
};
// ModulationShaping semantics depend on ModulationType.
constexpr uint8_t ModulationShaping{0x03};
enum DataModulModulationShapingFsk : uint8_t {
  ModulationShapingFskNone,
  ModulationShapingFskBt1p0,
  ModulationShapingFskBt0p5,
  ModulationShapingFskBt0p3,
};
enum DataModulModulationShapingOok : uint8_t {
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
enum Listen1ListenResolIdle : uint8_t {
  ListenResolIdle64us = 0x40,
  ListenResolIdle4100us = 0x80,
  ListenResolIdle262ms = 0xc0,
};
constexpr uint8_t ListenResolRx{0x30};
enum Listen1ListenResolRx : uint8_t {
  ListenResolRx64us = 0x10,
  ListenResolRx4100us = 0x20,
  ListenResolRx262ms = 0x30,
};
constexpr uint8_t ListenCriteria{0x08};
constexpr uint8_t ListenEnd{0x06};
enum Listen1ListenEnd : uint8_t {
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
enum LnaLnaZin : uint8_t {
  LnaZin50Ohms,
  LnaZin200Ohms = 0x80,
};
constexpr uint8_t LnaCurrentGain{0x38};
enum LnaLnaCurrentGain : uint8_t {
  LnaCurrentGain0db = 0x08,
  LnaCurrentGain6db = 0x10,
  LnaCurrentGain12db = 0x18,
  LnaCurrentGain24db = 0x20,
  LnaCurrentGain36db = 0x28,
  LnaCurrentGain48db = 0x30,
};
constexpr uint8_t LnaGainSelect{0x07};
enum LnaLnaGainSelect : uint8_t {
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
enum RxBwRxBwMant : uint8_t {
  RxBwMant16,
  RxBwMant20 = 0x08,
  RxBwMant24 = 0x10,
};
constexpr uint8_t RxBwExp{0x07};

// OokPeak
constexpr uint8_t OokThreshType{0xc0};
enum OokPeakOokThreshType : uint8_t {
  OokThreshTypeFixed,
  OokThreshTypePeak = 0x40,
  OokThreshTypeAverage = 0x80,
};
constexpr uint8_t OokPeakThreshStep{0x38};
enum OokPeakOokPeakThreshStep : uint8_t {
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
enum OokPeakOokPeakThreshDec : uint8_t {
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
enum OokAvgOokAverageThreshFilt : uint8_t {
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
enum DioMapping2ClkOut : uint8_t {
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
enum PacketConfig1PacketFormat : uint8_t {
  PacketFormatFixed,
  PacketFormatVariable = 0x80,
};
constexpr uint8_t DcFree{0x60};
enum PacketConfig1DcFree : uint8_t {
  DcFreeNone,
  DcFreeManchester = 0x20,
  DcFreeWhitening = 0x40,
};
constexpr uint8_t CrcOn{0x10};
constexpr uint8_t CrcAutoClearOff{0x08};
constexpr uint8_t AddressFiltering{0x06};
enum PacketConfig1AddressFiltering : uint8_t {
  AddressFilteringOff,
  AddressFilteringNode = 0x02,
  AddressFilteringBroadcast = 0x04,
};

// AutoModes
constexpr uint8_t EnterCondition{0xe0};
enum AutoModesEnterCondition : uint8_t {
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
enum AutoModesExitCondition : uint8_t {
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
enum AutoModesIntermediateMode : uint8_t {
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

}  // namespace Bits

}  // namespace rfm69hcw