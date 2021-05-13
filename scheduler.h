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
  static bool AddTask(const Task& task);

  // Runs the schedule of registered tasks.
  static void Run();

 private:
  static constexpr uint8_t kMaxTasks{1};

  Task task_[kMaxTasks]{};
  uint8_t count_{};

  // Scheduler singleton
  static Scheduler scheduler;
};