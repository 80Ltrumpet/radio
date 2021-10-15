#include "scheduler.h"

#include <avr/io.h>

#include "power.h"
#include "timer.h"

// Internal task struct
struct ScheduledTask final : public TaskInterface {
  ScheduledTask() = default;
  ScheduledTask(const Task&) = delete;
  ScheduledTask(Task&& t)
      : until{t.start_}, name{t.name_}, runner{t.runner_}, period{t.period_} {}
  
  void pause() override { until = Task::kPause; }
  void start(uint16_t delay_ms = 0) override {
    until = Timer::Millis() + delay_ms;
  }
  void set_period(uint16_t period_ms) override { period = period_ms; }

  uint64_t until{};
  const char* name{};
  Task::Runner runner{};
  uint16_t period{};
};

namespace {

constexpr uint8_t kMaxTasks{ANDRUIO_MAX_TASKS};

ScheduledTask tasks_[kMaxTasks]{};
uint8_t task_count_{};

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
  // If no tasks are run for at least 5 milliseconds, go to sleep.
  constexpr uint64_t kSpinTimeoutUs{5000};
  auto spin_us{Timer::Micros() + kSpinTimeoutUs};
  for (;;) {
    bool spinning{true};
    auto next_until{Task::kPause};
    for (uint8_t i{}; i < get_task_count(); ++i) {
      auto& task{tasks_[i]};
      // Skip tasks that are undefined, paused, or need to wait.
      if (!task.runner || task.until == Task::kPause) continue;
      if (Timer::Millis() < task.until) {
        // next_until is only meaningful when spinning.
        if (spinning &&
            (next_until == Task::kPause || task.until < next_until)) {
          next_until = task.until;
        }
        continue;
      }
      // We are not spinning if a task is run.
      spinning = false;
      auto until{task.until};
      task.runner();
      // The task may pause or reschedule itself while running.
      if (task.until != Task::kPause && task.until == until) {
        task.until = Timer::Millis() + task.period;
      }
    }

    // Sleep if no tasks have run for a while.
    if (spinning) {
      if (Timer::Micros() < spin_us) continue;
      if (next_until == Task::kPause) {
        Power::Sleep();
      } else if (auto now{Timer::Millis()}; now < next_until) {
        auto sleep_ms{next_until - now};
        Power::Sleep(sleep_ms < UINT16_MAX ? static_cast<uint16_t>(sleep_ms)
                                           : UINT16_MAX);
      }
      // Otherwise, we just happened to hit the spin timeout before the timer
      // rolled (extremely unlikely).
    }
    spin_us = Timer::Micros() + kSpinTimeoutUs;
  }
}

}  // namespace Scheduler