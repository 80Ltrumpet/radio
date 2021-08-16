#include "lsm6dsox.h"

#include <alloca.h>

#include "lsm6dsox_defs.h"
#include "twi.h"

namespace {

Twi::Device dev_{lsm6dsox::kI2cAddr};

}

namespace lsm6dsox {

// For conciseness
using namespace Bits;

bool Exists() {
  return dev_.read(Reg::WHO_AM_I) == Reg::Reset::WHO_AM_I;
}

void Configure() {
  // Disable I3C.
  dev_.write(Reg::CTRL9_XL, Reg::Reset::CTRL9_XL | I3C_DISABLE);

  // Burst write from FIFO_CTRL3 to INT1_CTRL, inclusive.
  uint8_t wbuf[Reg::INT1_CTRL - Reg::FIFO_CTRL3 + 1]{
    (BDR_0 << BDR_GY_) | BDR_12P5,
    DEC_TS_BATCH_NONE | ODR_T_BATCH_NONE | FIFO_MODE_STOP_ON_FULL,
    RST_COUNTER_BDR,
    15, // Number of batch events before a COUNTER_BDR_IA signal is generated
    INT1_CNT_BDR,
  };
  dev_.write(Reg::FIFO_CTRL3, wbuf, sizeof(wbuf));

  // Configure ultra-low-power mode.
  dev_.write(Reg::CTRL5_C, XL_ULP_EN);
}

void Enable() {
  // Use the highest, low-power ODR with +/-4g resolution.
  dev_.write(Reg::CTRL1_XL, ODR_XL_52 | FS_XL_4G);
}

void Disable() {
  dev_.write(Reg::CTRL1_XL, ODR_XL_OFF | FS_XL_4G);
}

void ProcessFifoData(void (*handler)(const FifoData&)) {
  // Keep going as long as there's something in the FIFO.
  uint16_t status;
  dev_.read(Reg::FIFO_STATUS1, &status, sizeof(status));
  auto diff{status & DIFF_FIFO};
  if (diff == 0) return;

  // Perform this allocation only once.
  const auto alloc_length{diff * sizeof(FifoData)};
  auto data{reinterpret_cast<FifoData*>(alloca(alloc_length))};
  do {
    auto length{diff * sizeof(FifoData)};
    // This is very unlikely, but let's be safe.
    if (length > alloc_length) {
      length = alloc_length;
      diff = length / sizeof(FifoData);
    }
    dev_.read(Reg::FIFO_DATA_OUT_TAG, data, length);
    for (uint8_t i{}; i < diff; ++i) handler(data[i]);

    dev_.read(Reg::FIFO_STATUS1, &status, sizeof(status));
    diff = status & DIFF_FIFO;
  } while (diff > 0);
}

}