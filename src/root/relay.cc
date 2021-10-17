#include "relay.h"

#include <avr/io.h>

#include "gpio.h"
#include "scheduler.h"

namespace {

Gpio control_{PINE, 4};
TaskHandle task_{};

void run() {
  control_.set();
  task_->pause();
}

}

namespace Relay {

void Init() {
  // The relay is active low.
  control_.set();
  control_.out();

  task_ = Scheduler::AddTask({"relay", run, 0, Task::kPause});
}

void SwitchOn() { control_.clear(); }

void SwitchOff(uint16_t delay_ms) { task_->start(delay_ms); }

}