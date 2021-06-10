#include "scheduler.h"

#include <avr/io.h>

#include "power.h"
#include "timer.h"

// Internal task struct
struct ScheduledTask final : public Task::Handle {
  ScheduledTask() = default;
  ScheduledTask(const Task&) = delete;
  ScheduledTask(Task&& t)
      : until{t.start_}, name{t.name_}, runner{t.runner_}, period{t.period_} {}

  uint64_t until{};
  const char* name{};
  Task::Runner runner{};
  uint16_t period{};
};

namespace {

constexpr uint8_t kMaxTasks{ANDRUIO_MAX_TASKS};

ScheduledTask tasks_[kMaxTasks]{};
uint8_t task_count_{};
uint8_t current_task_{kMaxTasks};

uint8_t get_task_count() {
  return task_count_ < kMaxTasks ? task_count_ : kMaxTasks;
}

}  // namespace

namespace Scheduler {

TaskHandle AddTask(Task&& task) {
  TaskHandle handle{};
  if (task_count_ < kMaxTasks) {
    tasks_[task_count_] = static_cast<Task&&>(task);
    handle = &tasks_[task_count_];
  }

  // Always increment the count to aid in verification of static provisioning.
  ++task_count_;
  return handle;
}

int8_t GetProvisioning() {
  return static_cast<int8_t>(kMaxTasks) - static_cast<int8_t>(task_count_);
}

void Run() {
  // If no tasks are run for at least 10 milliseconds, go to sleep.
  constexpr uint64_t kSpinTimeoutUs{10000};
  const auto kTaskCount{get_task_count()};
  auto spin_us{Timer::Micros() + kSpinTimeoutUs};
  for (;;) {
    bool spinning{false};
    uint64_t next_until{};
    for (current_task_ = 0; current_task_ < kTaskCount; ++current_task_) {
      auto& task{tasks_[current_task_]};
      // Skip tasks that are undefined, paused, or need to wait.
      if (!task.runner || task.until == Task::kPause) continue;
      if (Timer::Millis() < task.until) {
        if (next_until == 0 || task.until < next_until) next_until = task.until;
        continue;
      }
      spinning = false;
      // TODO: Need to handle long periods.
      task.runner();
      // The task may pause itself while running.
      if (task.until != Task::kPause) {
        task.until = Timer::Millis() + task.period;
        if (next_until == 0 || task.until < next_until) next_until = task.until;
      }
    }

    // Sleep if no tasks have run for a while.
    if (spinning) {
      if (Timer::Micros() >= spin_us) {
        if (next_until == 0) {
          Power::Sleep();
        } else if (auto now{Timer::Millis()}; now < next_until) {
          auto sleep_ms{next_until - now};
          Power::Sleep(sleep_ms < UINT16_MAX ? static_cast<uint16_t>(sleep_ms)
                                             : UINT16_MAX);
        }
        // Otherwise, we just happened to hit the spin timeout before the timer
        // rolled (extremely unlikely).
        goto reset_spin_us;
      }
    } else {
    reset_spin_us:
      spin_us = Timer::Micros() + kSpinTimeoutUs;
    }
  }
}

void PauseTask(TaskHandle handle) {
  auto& task{*reinterpret_cast<ScheduledTask*>(handle)};
  task.until = Task::kPause;
}

void RestartTask(TaskHandle handle) {
  auto& task{*reinterpret_cast<ScheduledTask*>(handle)};
  task.until = Timer::Millis();
}

void SetTaskPeriod(TaskHandle handle, uint16_t period_ms) {
  auto& task{*reinterpret_cast<ScheduledTask*>(handle)};
  task.period = period_ms;
}

}  // namespace Scheduler