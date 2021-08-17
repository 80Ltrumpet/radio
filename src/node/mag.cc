#include "mag.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#include "command_registry.h"
#include "lis3mdl.h"
#include "scheduler.h"
#include "twi.h"
#include "vec3.h"

using namespace lis3mdl;

using RawSample = Vec3<int16_t>;

struct Sample final : public Vec3<float> {
  Sample() = default;
  explicit Sample(const RawSample& raw);
};

namespace {

constexpr auto kFullScale{Bits::FS_4GAUSS};
constexpr int16_t kLsbPerGauss{OutLsbPerGauss(kFullScale)};

Twi::Device dev_{kI2cAddr};
bool exists_{false};
bool sampling_{false};
TaskHandle task_{};

void run() {
  RawSample raw;
  do {
    dev_.read(Reg::OUT_X_L, &raw, sizeof(raw));
    Sample sample{raw};
    printf("%.4f %.4f %.4f\n", sample.x, sample.y, sample.z);
  } while (dev_.read(Reg::STATUS) & Bits::ZYXDA);
  task_->pause();  // Always started by interrupt
}

}  // namespace

Sample::Sample(const RawSample& raw) {
  for (uint8_t i{}; i < 3; ++i)
    v[i] = static_cast<float>(raw.v[i]) / kLsbPerGauss;
}

namespace Mag {

void Init() {
  // Check if the device exists.
  if (!(exists_ = dev_.read(Reg::WHO_AM_I) == Reg::Reset::WHO_AM_I)) {
    return;
  }

  // Configure the external interrupt (rising edge).
  EICRA |= _BV(ISC21) | _BV(ISC20);
  EIMSK |= _BV(INT2);

  // Configure low-power operation, sampling at 5 Hz, with block-data update.
  uint8_t ctrl[]{ Bits::OM_LP | Bits::DO_5HZ, kFullScale };
  dev_.write(Reg::CTRL1, ctrl, sizeof(ctrl));
  ctrl[0] = Bits::BDU;
  dev_.write(Reg::CTRL5, ctrl[0]);

  task_ = Scheduler::AddTask({"mag", run, 0, Task::kPause});
}

}  // namespace Mag

// TODO: Right now, this command only exists for debugging.
struct MagCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void MagCommand::CommandHandler(int argc, const char* argv[]) {
  if (!exists_) {
    printf("There is no magnetometer.\n");
    return;
  }
  if (sampling_) {
    dev_.write(Reg::CTRL3, Bits::MD_POWER_DOWN);
  } else {
    dev_.write(Reg::CTRL3, Bits::MD_CONTINUOUS);
    task_->start();
  }
  sampling_ = !sampling_;
}

const char* const MagCommand::kCommandName{"mag"};
const bool MagCommand::registered{
    CommandRegistry::RegisterCommand<MagCommand>()};

ISR(INT2_vect) {
  task_->start();
}