#pragma once

#include <stdint.h>

class Task final {
  friend class ScheduledTask;
  using Runner = void (*)(void*);

 public:
  static constexpr uint64_t kPause{};
  static constexpr uint64_t kStart{1};

  Task(const Task&) = delete;
  Task(const char* name, Runner runner, uint16_t period = 0,
       void* arg = nullptr, uint64_t start = kStart)
      : start_{start},
        name_{name},
        runner_{runner},
        arg_{arg},
        period_{period} {}

 private:
  uint64_t start_;
  const char* name_;
  Runner runner_;
  void* arg_;
  uint16_t period_;
};

namespace Scheduler {

// Registers a task's parameters with the scheduler.
void AddTask(Task&& task);

// Returns the number of extra task "slots" in the scheduler. If it is
// negative, some tasks were unable to be added (under-provisioned).
int8_t GetProvisioning();

// Runs the schedule of registered tasks.
void Run();

}  // namespace Scheduler