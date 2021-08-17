#include "imu.h"

#include <alloca.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#include "command_registry.h"
#include "lsm6dsox.h"
#include "scheduler.h"
#include "twi.h"
#include "vec3.h"

struct FifoData final {
  union Tag {
    struct {
      const uint8_t parity : 1;
      const uint8_t counter : 2;
      const uint8_t sensor : 5;
    };
    const uint8_t value;
  };

  using RawSample = Vec3<int16_t>;
  using Sample = Vec3<float>;

  Sample get_sample() const;

  Tag tag;
  uint8_t payload[sizeof(RawSample)];
};
static_assert(sizeof(FifoData) == 7, "FifoData has incorrect size");

using namespace lsm6dsox;

namespace {

constexpr auto kAccelFullScale{Bits::FS_XL_2G};
constexpr int16_t kLsbPerG{INT16_MAX >> 1};

Twi::Device dev_{kI2cAddr};
bool exists_{false};
bool sampling_{false};
TaskHandle task_{};

void run() {
  uint16_t status;
  dev_.read(Reg::FIFO_STATUS1, &status, sizeof(status));
  auto diff{status & Bits::DIFF_FIFO};
  if (diff == 0) {
    return;
  }

  // Allocate space on the stack for reading the FIFO.
  const auto alloc_length{diff * sizeof(FifoData)};
  auto data{reinterpret_cast<FifoData*>(alloca(alloc_length))};
  do {
    auto length{diff * sizeof(FifoData)};
    // This is very unlikely, but it is included for safety.
    if (length > alloc_length) {
      length = alloc_length;
      diff = length / sizeof(FifoData);
    }
    dev_.read(Reg::FIFO_DATA_OUT_TAG, data, length);
    for (uint16_t i{}; i < diff; ++i) {
      const auto sample{data[i].get_sample()};
      printf("%.4f %.4f %.4f\n", sample.x, sample.y, sample.z);
    }

    dev_.read(Reg::FIFO_STATUS1, &status, sizeof(status));
    diff = status & Bits::DIFF_FIFO;
  } while (diff > 0);

  task_->pause();
}

}  // namespace

FifoData::Sample FifoData::get_sample() const {
  const auto& raw{*reinterpret_cast<const RawSample*>(payload)};
  Sample sample;
  // TODO: Use tag.sensor to determine proper conversion.
  // FIXME: For now, just assume accel data.
  for (uint8_t i{}; i < 3; ++i) {
    sample.v[i] = static_cast<float>(raw.v[i]) / kLsbPerG;
  }
  return sample;
}

namespace Imu {

void Init() {
  // Check if the device exists.
  if (!(exists_ = dev_.read(Reg::WHO_AM_I) == Reg::Reset::WHO_AM_I)) {
    return;
  }

  // Configure the external interrupt (rising edge).
  EICRA |= _BV(ISC31) | _BV(ISC30);
  EIMSK |= _BV(INT3);

  // Disable I3C.
  dev_.write(Reg::CTRL9_XL, Reg::Reset::CTRL9_XL | Bits::I3C_DISABLE);

  // Burst write from FIFO_CTRL3 to INT1_CTRL, inclusive.
  uint8_t wbuf[Reg::COUNTER_BDR_REG2 - Reg::FIFO_CTRL3 + 1]{
    (Bits::BDR_0 << Bits::BDR_GY_) | Bits::BDR_12P5,
    Bits::DEC_TS_BATCH_NONE | Bits::ODR_T_BATCH_NONE |
      Bits::FIFO_MODE_STOP_ON_FULL,
    Bits::RST_COUNTER_BDR,
    15, // Number of batch events before a COUNTER_BDR_IA signal is generated
  };
  dev_.write(Reg::FIFO_CTRL3, wbuf, sizeof(wbuf));

  // COUNTER_BDR_IA on INT2
  dev_.write(Reg::INT2_CTRL, Bits::INT2_CNT_BDR);

  // Configure ultra-low-power mode.
  dev_.write(Reg::CTRL5_C, Bits::XL_ULP_EN);
  
  task_ = Scheduler::AddTask({"imu", run, 0, Task::kPause});
}

}  // namespace Imu

// TODO: Right now, this command only exists for debugging.
struct ImuCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void ImuCommand::CommandHandler(int argc, const char* argv[]) {
  if (!exists_) {
    printf("There is no IMU.\n");
    return;
  }
  if (sampling_) {
    dev_.write(Reg::CTRL1_XL, Bits::ODR_XL_OFF | kAccelFullScale);
  } else {
    // Use the highest, low-power ODR with +/-2g resolution.
    dev_.write(Reg::CTRL1_XL, Bits::ODR_XL_52 | kAccelFullScale);
    task_->start();
  }
  sampling_ = !sampling_;
}

const char* const ImuCommand::kCommandName{"imu"};
const bool ImuCommand::registered{
    CommandRegistry::RegisterCommand<ImuCommand>()};

ISR(INT3_vect) {
  task_->start();
}