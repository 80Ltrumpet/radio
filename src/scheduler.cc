#include "scheduler.h"

#include <avr/io.h>

#include "power.h"
#include "timer.h"

#if 1  // DEBUG
#include <stdio.h>
#endif

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
  // TODO: Right now, this sleep strategy is fine, but it will not work well if
  // a task has a long period. This will be true when the radio network is
  // implemented, as it will need to periodically send packets, check ACK
  // timeouts, etc. This can be handled as follows:
  //
  // If a task has run before the spin timeout:
  //   Reset the spin timeout.
  // Else:
  //   If the next task time can be configured in a timer:
  //     Configure a timer to fire an interrupt after the intervening duration.
  //   Else:
  //     Configure a timer to fire after the largest interval possible.
  //   Sleep.
  // On wake:
  //   Add the equivalent number of milliseconds to Timer::millis_.
  //   (NOTE: This will need to be entirely based on the state of the timer's
  //   registers, most likely TCNTX or whatever else can be incremented without
  //   waking up the MCU.)

  // If all tasks are paused for at least 10 milliseconds, go to sleep.
  constexpr uint64_t kSleepTimeoutUs{10000};
  const auto kTaskCount{get_task_count()};
  auto next_sleep_us{Timer::Micros() + kSleepTimeoutUs};
  for (;;) {
    bool paused{false};
    for (current_task_ = 0; current_task_ < kTaskCount; ++current_task_) {
      auto& task{tasks_[current_task_]};
      // Skip tasks that are undefined, paused, or need to wait.
      auto task_paused{!task.runner || task.until == Task::kPause};
      paused = paused || task_paused;
      if (task_paused || Timer::Millis() < task.until) {
        continue;
      }
      task.runner();
      // The task may pause itself while running.
      if (task.until != Task::kPause) {
        task.until = Timer::Millis() + task.period;
      }
    }

    // Sleep if all tasks have been paused for a while.
    if (paused) {
      if (Timer::Micros() >= next_sleep_us) {
        Power::SleepIdle();
        next_sleep_us = Timer::Micros() + kSleepTimeoutUs;
      }
    } else {
      next_sleep_us = Timer::Micros() + kSleepTimeoutUs;
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