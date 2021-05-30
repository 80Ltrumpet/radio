#include "scheduler.h"

#include "timer.h"

// Internal task struct
struct ScheduledTask final {
  ScheduledTask() = default;
  ScheduledTask(const Task&) = delete;
  ScheduledTask(Task&& t)
      : until{t.start_},
        name{t.name_},
        runner{t.runner_},
        arg{t.arg_},
        period{t.period_} {}

  uint64_t until{};
  const char* name{};
  Task::Runner runner{};
  void* arg{};
  uint16_t period{};
};

namespace {

constexpr uint8_t kMaxTasks{ANDRUIO_MAX_TASKS};

ScheduledTask g_tasks[kMaxTasks]{};
uint8_t g_task_count{};

}  // namespace

namespace Scheduler {

void AddTask(Task&& task) {
  if (g_task_count < kMaxTasks) {
    g_tasks[g_task_count] = static_cast<Task&&>(task);
  }

  // Always increment the count to aid in verification of static provisioning.
  ++g_task_count;
}

int8_t GetProvisioning() {
  return static_cast<int8_t>(kMaxTasks) - static_cast<int8_t>(g_task_count);
}

void Run() {
  const auto task_count{g_task_count < kMaxTasks ? g_task_count : kMaxTasks};
  for (;;) {
    for (uint8_t i{}; i < task_count; ++i) {
      auto& task{g_tasks[i]};
      // Skip tasks that are undefined, paused, or need to wait.
      if (!task.runner || task.until == Task::kPause ||
          Timer::Millis() < task.until) {
        continue;
      }
      task.runner(task.arg);
      task.until = Timer::Millis() + task.period;
    }
  }
}

}  // namespace Scheduler