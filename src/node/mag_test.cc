#include <stdio.h>
#include <string.h>

#include "command_registry.h"
#include "lis3mdl.h"
#include "scheduler.h"
#include "timer.h"

namespace {

constexpr uint8_t kPollCountLimit{10};
constexpr uint16_t kPollTimeoutMs{5000};

TaskHandle task_{};
bool configured_{false};
bool enabled_{false};
uint8_t poll_count_{};
uint64_t poll_timeout_ms_{};

void run() {
  if (!enabled_) {
    lis3mdl::Enable();
    enabled_ = true;
    poll_timeout_ms_ = Timer::Millis() + kPollTimeoutMs;
    task_->set_period(200);
    return;
  }

  if (lis3mdl::Available()) {
    ++poll_count_;
    auto sample{lis3mdl::Poll()};
    printf("\r%" PRIx16 ", %" PRIx16 ", %" PRIx16 "\n", sample.x, sample.y,
           sample.z);
    task_->set_period(200);
  } else {
    // Try again in 10 milliseconds.
    task_->set_period(10);
  }

  if (poll_count_ >= kPollCountLimit || Timer::Millis() >= poll_timeout_ms_) {
    lis3mdl::Disable();
    enabled_ = false;
    task_->pause();
  }
}

}  // namespace

struct MagTestCommand final {
  static void CommandHandler(int argc, const char* argv[]);
  static const char* const kCommandName;

 private:
  static const bool registered;
};

void MagTestCommand::CommandHandler(int argc, const char* argv[]) {
  // Lazily initialize the task.
  if (!task_) {
    task_ = Scheduler::AddTask({"mag_test", run, 200, Task::kPause});
  }

  if (argc == 1) {
  usage:
    puts(
        "Usage: mag exists\n"
        "           poll");
    return;
  }

  if (strcmp(argv[1], "exists") == 0) {
    printf("%s\n", lis3mdl::Exists() ? "true" : "false");
  } else if (strcmp(argv[1], "poll") == 0) {
    if (!configured_) {
      lis3mdl::Configure();
      configured_ = true;
    }
    poll_count_ = 0;
    task_->start();
  } else {
    goto usage;
  }
}

const char* const MagTestCommand::kCommandName{"mag"};
const bool MagTestCommand::registered{
    CommandRegistry::RegisterCommand<MagTestCommand>()};
