#pragma once

#include <stdint.h>

class Task final {
  friend class ScheduledTask;
  using Runner = void (*)();

 public:
  static constexpr uint64_t kPause{};
  static constexpr uint64_t kStart{1};

  struct Handle {};

  Task(const Task&) = delete;
  Task(const char* name, Runner runner, uint16_t period_ms = 0,
       uint64_t start = kStart)
      : start_{start},
        name_{name},
        runner_{runner},
        period_{period_ms} {}

 private:
  uint64_t start_;
  const char* name_;
  Runner runner_;
  uint16_t period_;
};

using TaskHandle = Task::Handle*;

namespace Scheduler {

// Registers a task's parameters with the scheduler.
TaskHandle AddTask(Task&& task);

// Returns the number of extra task "slots" in the scheduler. If it is
// negative, some tasks were unable to be added (under-provisioned).
int8_t GetProvisioning();

// Runs the schedule of registered tasks.
void Run();

void PauseTask(TaskHandle handle);
void RestartTask(TaskHandle handle);
void SetTaskPeriod(TaskHandle handle, uint16_t period_ms);

}  // namespace Scheduler