#include <stdio.h>
#include <string.h>

#include "command_registry.h"
#include "lsm6dsox.h"
#include "scheduler.h"
#include "timer.h"

namespace {

constexpr uint8_t kPollCountLimit{10};
constexpr uint16_t kPollTimeoutMs{5000};
constexpr float kResolution{8192.0f};

TaskHandle task_{};
bool configured_{false};
bool enabled_{false};
uint8_t poll_count_{};
uint64_t poll_timeout_ms_{};

void poll(const lsm6dsox::FifoData& data) {
  ++poll_count_;
  const auto& sample{data.get_sample()};
  printf("\r%02x (%.8f, %.8f, %.8f)\n", data.tag.value, sample.x / kResolution,
         sample.y / kResolution, sample.z / kResolution);
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
