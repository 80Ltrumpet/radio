#include "scheduler.h"

#include "timer.h"

// Scheduler singleton
Scheduler Scheduler::scheduler{};

void Scheduler::AddTask(const Task& task) {
  if (scheduler.count_ < kMaxTasks) {
    scheduler.task_[scheduler.count_] = task;
  }

  // Always increment the count to aid in verification of static provisioning.
  ++scheduler.count_;
}

int8_t Scheduler::GetProvisioning() {
  return static_cast<int8_t>(kMaxTasks) - static_cast<int8_t>(scheduler.count_);
}

void Scheduler::Run() {
  const auto task_count{scheduler.count_ < kMaxTasks ? scheduler.count_
                                                     : kMaxTasks};
  for (;;) {
    for (uint8_t i{}; i < task_count; ++i) {
      auto& task{scheduler.task_[i]};
      // Skip tasks that are either paused or need to wait.
      if (task.until == 0 || Timer::Millis() < task.until) {
        continue;
      }
      task.runner(task.arg);
      task.until = Timer::Millis() + task.period;
    }
  }
}