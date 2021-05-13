#include "scheduler.h"

#include "clock.h"

// Scheduler singleton
Scheduler Scheduler::scheduler{};

bool Scheduler::AddTask(const Task& task) {
  auto success{scheduler.count_ < kMaxTasks};
  if (success) {
    scheduler.task_[scheduler.count_] = task;
  }

  // TODO: Perform this static provisioning verification somewhere...
  // Always increment the count to aid in verification of static provisioning.
  ++scheduler.count_;
  return success;
}

void Scheduler::Run() {
  const auto task_count{scheduler.count_ < kMaxTasks ? scheduler.count_
                                                     : kMaxTasks};
  for (;;) {
    for (uint8_t i{}; i < task_count; ++i) {
      auto& task{scheduler.task_[i]};
      // Skip tasks that are either paused or need to wait.
      if (task.until == 0 || Clock::GetMillis() < task.until) {
        continue;
      }
      task.runner(task.arg);
      task.until = Clock::GetMillis() + task.period;
    }
  }
}