#pragma once

#include <stdint.h>

class Scheduler final {
  using TaskRunner = void (*)(void*);

 public:
  struct Task final {
    const char* name{};
    TaskRunner runner{};
    void* arg{};
    unsigned long period{};
    unsigned long until{1UL};
  };

  // Registers a task's parameters with the scheduler.
  static void AddTask(const Task& task);

  // Returns the number of extra task "slots" in the scheduler. If it is
  // negative, some tasks were unable to be added (under-provisioned).
  static int8_t GetProvisioning();

  // Runs the schedule of registered tasks.
  static void Run();

 private:
  static constexpr uint8_t kMaxTasks{ANDRUIO_MAX_TASKS};

  Scheduler() = default;

  Task task_[kMaxTasks]{};
  uint8_t count_{};

  // Scheduler singleton
  static Scheduler scheduler;
};