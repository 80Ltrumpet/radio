#ifndef ANDRUIO_CONFIG_ROOT

#include <stdio.h>
#include <string.h>

#include "command_registry.h"
#include "lsm6dsox.h"
#include "scheduler.h"
#include "timer.h"

namespace {

constexpr uint8_t kPollCountLimit{10};
constexpr uint16_t kPollTimeoutMs{5000};
constexpr int32_t kResolution{12207};

TaskHandle task_{};
bool configured_{false};
bool enabled_{false};
uint8_t poll_count_{};
uint64_t poll_timeout_ms_{};

void poll(const lsm6dsox::FifoDatum& datum) {
  ++poll_count_;
#if 0  // DEBUG
  int32_t x{datum.x() * kResolution};
  int32_t y{datum.y() * kResolution};
  int32_t z{datum.z() * kResolution};
  printf("\r%02" PRIx8 " (%" PRId32 ".%08" PRId32 ", %" PRId32 ".%08" PRId32
         ", %" PRId32 ".%08" PRId32 ")\n",
         datum.tag, x / 100000000, +x % 100000000, y / 100000000,
         +y % 100000000, z / 100000000, +z % 100000000);
#else
  printf("\r%02x (%02x%02x, %02x%02x, %02x%02x)\n", datum.tag, datum.xh,
         datum.xl, datum.yh, datum.yl, datum.zh, datum.zl);
#endif
}

void run() {
  if (!enabled_) {
    lsm6dsox::Enable();
    enabled_ = true;
    poll_timeout_ms_ = Timer::Millis() + kPollTimeoutMs;
    return;
  }

  lsm6dsox::ProcessFifoData(poll);

  if (poll_count_ >= kPollCountLimit || Timer::Millis() >= poll_timeout_ms_) {
    lsm6dsox::Disable();
    enabled_ = false;
    task_->pause();
  }
}

}  // namespace

struct ImuTestCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void ImuTestCommand::CommandHandler(int argc, const char* argv[]) {
  // Lazily initialize the task.
  if (!task_) {
    task_ = Scheduler::AddTask({"imu_test", run, 500, Task::kPause});
  }

  if (argc == 1) {
  usage:
    puts(
        "Usage: imu exists\n"
        "           poll");
    return;
  }

  if (strcmp(argv[1], "exists") == 0) {
    printf("%s\n", lsm6dsox::Exists() ? "true" : "false");
  } else if (strcmp(argv[1], "poll") == 0) {
    if (!configured_) {
      lsm6dsox::Configure();
      configured_ = true;
    }
    poll_count_ = 0;
    task_->start();
  } else {
    goto usage;
  }
}

const char* const ImuTestCommand::kCommandName{"imu"};
const bool ImuTestCommand::registered{
    CommandRegistry::RegisterCommand<ImuTestCommand>()};

#endif